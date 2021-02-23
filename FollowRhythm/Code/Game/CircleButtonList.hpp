#pragma once

#include "Game/ButtonList.hpp"

// selections would be looped
class CircleButtonList : public ButtonList
{
public:
    CircleButtonList();

    void Render(std::vector<Vertex_PCU>& textVerts) const override;

public:
    unsigned int m_showLines = 3;
    Vec2 m_singleBoundDim;
    AABB2 m_generalDrawBounds;
    Vec2 m_textAlignment = Vec2(.3f, .5f);
};