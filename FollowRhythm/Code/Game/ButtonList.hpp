#pragma once

#include <string>
#include <vector>
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"

class Texture;
class Timer;
class XboxController;
struct Vertex_PCU;

void InitButtonAssets();

//////////////////////////////////////////////////////////////////////////
struct Button
{
public:
    Button(std::string const& content, bool selected, AABB2 const& bounds);

public:
    std::string text;
    bool isSelected = false;
    AABB2 drawBounds;
};

//////////////////////////////////////////////////////////////////////////
class ButtonList
{
public:
    ButtonList();

    void UpdateForSelection();
    void UpdateForNavigationInput(XboxController const& controller);
    virtual void Render(std::vector<Vertex_PCU>& textVerts) const;

public:
    std::vector<Button> m_buttons;
    unsigned int m_selectedIndex = 0;
    float m_highlightScale = 1.2f;
    Rgba8 m_textColor = Rgba8(250,250,250);
    Rgba8 m_highlightColor = Rgba8(250,250,100);
    Rgba8 m_greyColor = Rgba8(200,200,200);
    Texture* m_buttonTex = nullptr;

protected:
    bool m_isUp = true;
    Timer* m_timer = nullptr;

    void UpdateForJoystickInput(XboxController const& controller);
    void UpdateForDpadInput(XboxController const& controller);

    void NavigateUpOnce();
    void NavigateDownOnce();
};