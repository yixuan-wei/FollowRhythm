#pragma once

#include "Game/Note.hpp"
#include "Engine/Core/EventSystem.hpp"
#include <vector>

class Timer;
class Emitter2D;

class MultiNotes : public Note
{
public:
    MultiNotes(Song* song, bool isLeft, bool isUp, unsigned int startMS, unsigned int duration);
    ~MultiNotes() = default;

    void StartToSee() override;
    void EndToSee() override;
    void Render(AABB2 const& bounds) const override;

    unsigned int GetRenderBeginMS() const override;
    unsigned int GetRenderEndMS() const override;
    bool IsScored() const override;

    bool HandleJoystickMoved(EventArgs& args);

private:
    unsigned int m_duration = 0;
    bool m_isUp = true;
    unsigned int m_actualStart = 0;
    unsigned int m_actualEnd = 0;
    Emitter2D* m_emitter = nullptr;
};