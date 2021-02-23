#pragma once

#include "Game/Note.hpp"
#include "Engine/Core/EventSystem.hpp"

class SingleNote : public Note
{
public:
    SingleNote(Song* song, bool isLeft, unsigned int startTimeSeconds);
    ~SingleNote() = default;

    void StartToSee() override;
    void EndToSee() override;
    void Render(AABB2 const& bounds) const override;

    unsigned int GetRenderBeginMS() const override;
    unsigned int GetRenderEndMS() const override;
    bool IsScored() const override;

    bool HandleButtonPressed(EventArgs& args);

private:
    bool m_isHit = false;
};