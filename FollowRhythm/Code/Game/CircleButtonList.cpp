#include "Game/CircleButtonList.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/RenderContext.hpp"

//////////////////////////////////////////////////////////////////////////
CircleButtonList::CircleButtonList()
    : ButtonList()
{
}

static void AppendAABB2ToVertsArrayWithColor(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, 
    Rgba8 const& blColor, Rgba8 const& brColor,
    Rgba8 const& trColor, Rgba8 const& tlColor)
{
    //TODO add tesselate
    verts.push_back(Vertex_PCU(bounds.mins, blColor));
    verts.push_back(Vertex_PCU(Vec2(bounds.maxs.x, bounds.mins.y), brColor, Vec2(1.f,0.f)));
    verts.push_back(Vertex_PCU(bounds.maxs, trColor, Vec2::ONE));

    verts.push_back(Vertex_PCU(bounds.mins, blColor));
    verts.push_back(Vertex_PCU(bounds.maxs, trColor, Vec2::ONE));
    verts.push_back(Vertex_PCU(Vec2(bounds.mins.x, bounds.maxs.y), tlColor, Vec2(0.f, 1.f)));
}

//////////////////////////////////////////////////////////////////////////
void CircleButtonList::Render(std::vector<Vertex_PCU>& textVerts) const
{
    std::vector<Vertex_PCU> bgVerts;
    Vec2 generalDim = m_generalDrawBounds.GetDimensions();
    float lineNum = (float)m_showLines;
    float singleHeight = generalDim.y/(lineNum+2.f);
    Vec2 singleDim = m_singleBoundDim;
    singleDim.y = singleHeight;
    float singleGap = singleHeight/(lineNum-1.f);
    Vec2 deltaTrans(0.f, singleGap + singleHeight);
    AABB2 singleBounds(m_generalDrawBounds.mins, m_generalDrawBounds.mins+singleDim);
    float textHeight = singleHeight*.4f;

    Rgba8 greyWhite = m_greyColor;
    Rgba8 transWhite = greyWhite;
    transWhite.a = 0;
    unsigned int buttonNum = (unsigned int)m_buttons.size();

    //bottom
    unsigned int bottomIdx = m_selectedIndex==buttonNum-1? 0: m_selectedIndex+1;
    g_theFont->AddVertsForTextInBox2D(textVerts, singleBounds, textHeight, m_buttons[bottomIdx].text,
        Rgba8::WHITE, FONT_DEFAULT_ASPECT, m_textAlignment, .05f, FONT_DEFAULT_KERNING);
    AppendAABB2ToVertsArrayWithColor(bgVerts, singleBounds, greyWhite, transWhite, transWhite, greyWhite);

    //highlight
    singleBounds.Translate(deltaTrans+Vec2(0.f, singleHeight*.5f));
    AABB2 highlightBounds = singleBounds;
    highlightBounds.SetDimensions(1.8f*singleDim);
    g_theFont->AddVertsForTextInBox2D(textVerts, m_generalDrawBounds, textHeight*2.f, m_buttons[m_selectedIndex].text,
        Rgba8::WHITE, FONT_DEFAULT_ASPECT, Vec2(.1f, .5f), .05f, FONT_DEFAULT_KERNING);
    Rgba8 transHighlight = m_highlightColor;
    transHighlight.a = 0;
    AppendAABB2ToVertsArrayWithColor(bgVerts, highlightBounds, m_highlightColor, transHighlight, transHighlight, m_highlightColor);

    //top
    unsigned int topIdx = m_selectedIndex==0?buttonNum-1:m_selectedIndex-1;
    singleBounds.Translate(deltaTrans+Vec2(0.f,singleHeight*.5f));
    g_theFont->AddVertsForTextInBox2D(textVerts, singleBounds, textHeight, m_buttons[topIdx].text,
        Rgba8::WHITE, FONT_DEFAULT_ASPECT, m_textAlignment, .05f, FONT_DEFAULT_KERNING);
    AppendAABB2ToVertsArrayWithColor(bgVerts, singleBounds, greyWhite, transWhite, transWhite, greyWhite);

    //draw
    g_theRenderer->BindDiffuseTexture(m_buttonTex);
    g_theRenderer->DrawVertexArray(bgVerts);
}
