#include "Game/ButtonList.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Input/XboxController.hpp"

SoundID sButtonSFXID;

//////////////////////////////////////////////////////////////////////////
Button::Button(std::string const& content, bool selected, AABB2 const& bounds)
    : text(content)
    , isSelected(selected)
    , drawBounds(bounds)
{
    
}

//////////////////////////////////////////////////////////////////////////
ButtonList::ButtonList()
{
    m_timer = new Timer();
}

//////////////////////////////////////////////////////////////////////////
void ButtonList::UpdateForNavigationInput(XboxController const& controller)
{    
   UpdateForJoystickInput(controller);
   UpdateForDpadInput(controller);    

    UpdateForSelection();
}

//////////////////////////////////////////////////////////////////////////
void ButtonList::Render(std::vector<Vertex_PCU>& textVerts) const
{
    //draw background
    std::vector<Vertex_PCU> bgVerts;
    for(Button const& button : m_buttons){
        Rgba8 color = m_greyColor;
        AABB2 bounds = button.drawBounds;
        Vec2 dim = bounds.GetDimensions();
        float textSize = dim.y*.4f;
        if (button.isSelected) {
            color = m_highlightColor;
            dim *= m_highlightScale;
            bounds.SetDimensions(dim);
            textSize = dim.y*.5f;
        }
        g_theFont->AddVertsForTextInBox2D(textVerts, bounds, textSize, button.text, m_textColor, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .05f, FONT_DEFAULT_KERNING);
        AppendVertsForAABB2D(bgVerts, bounds, Vec2::ZERO, Vec2::ONE, color);
    }
    g_theRenderer->BindDiffuseTexture(m_buttonTex);
    g_theRenderer->DrawVertexArray(bgVerts);    
}

//////////////////////////////////////////////////////////////////////////
void ButtonList::UpdateForJoystickInput(XboxController const& controller)
{
    AnalogJoystick const& lJoystick = controller.GetLeftJoystick();
    float lStickYValue = lJoystick.GetPosition().y;

    if (AbsFloat(lStickYValue) < INPUT_JOYSTICK_DEAD_Y) {
        m_timer->Stop();
        return;
    }

    if (!m_timer->IsRunning()) {
        m_isUp = lStickYValue > 0.f;
        m_timer->SetTimerSeconds(g_theGame->GetGameClock(), 0.3);
        if (m_isUp) {
            NavigateUpOnce();
        }
        else {
            NavigateDownOnce();
        }
    }
    else if (m_timer->HasElapsed()) {
        m_timer->Stop();
    }
}

//////////////////////////////////////////////////////////////////////////
void ButtonList::UpdateForDpadInput(XboxController const& controller)
{
    if (controller.GetButtonState(XBOX_BUTTON_ID_DPAD_UP).WasJustPressed()) {
        NavigateUpOnce();
    }
    if (controller.GetButtonState(XBOX_BUTTON_ID_DPAD_DOWN).WasJustPressed()) {
        NavigateDownOnce();
    }
}

//////////////////////////////////////////////////////////////////////////
void ButtonList::NavigateUpOnce()
{
    unsigned int lastIndex = (unsigned int)(m_buttons.size() - 1);
    m_selectedIndex = m_selectedIndex == 0 ? lastIndex : m_selectedIndex - 1;
    g_theAudio->PlaySound(sButtonSFXID, false, gSFXVolume * .8f);
}

//////////////////////////////////////////////////////////////////////////
void ButtonList::NavigateDownOnce()
{
    unsigned int lastIndex = (unsigned int)(m_buttons.size() - 1);
    m_selectedIndex = m_selectedIndex == lastIndex ? 0 : m_selectedIndex + 1;
    g_theAudio->PlaySound(sButtonSFXID, false, gSFXVolume * .8f);
}

//////////////////////////////////////////////////////////////////////////
void ButtonList::UpdateForSelection()
{
    size_t curIndex = (size_t)m_selectedIndex;
    for (size_t i = 0; i < m_buttons.size(); i++) {
        if (curIndex == i) {
            m_buttons[i].isSelected=true;
        }
        else {
            m_buttons[i].isSelected=false;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void InitButtonAssets()
{
    sButtonSFXID = g_theAudio->CreateOrGetSound("Data/music/sounds/button.ogg");
    gButtonSFXID = g_theAudio->CreateOrGetSound("data/music/sounds/click.wav");
}
