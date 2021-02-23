#include "Game/MultiNotes.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Song.hpp"
#include "Game/Effects.hpp"
#include "Game/AssetManager.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/Emitter2D.hpp"

#include "Game/Game.hpp"

static float sNoteHitPosLeftX = 0.f;
static float sNoteHitPosRightX = 0.f;

//////////////////////////////////////////////////////////////////////////
MultiNotes::MultiNotes(Song* song, bool isLeft, bool isUp, unsigned int startMS, unsigned int duration)
    : Note(song, NOTE_MULTIPLE, isLeft, startMS)
    , m_duration(duration)
    , m_isUp(isUp)
{
}

//////////////////////////////////////////////////////////////////////////
void MultiNotes::StartToSee()
{
    g_theEvents->RegisterMethodEvent("JoystickMoved", this, &MultiNotes::HandleJoystickMoved, "joystick moved", EVENT_GAME);
}

//////////////////////////////////////////////////////////////////////////
void MultiNotes::EndToSee()
{
    //score
    if(m_actualStart>0){
        if (m_actualEnd == 0) {
            m_actualEnd = m_song->GetSongElapsedMS();
        }
        float duration = (float)m_duration;
        float score = (float)(m_actualEnd - m_actualStart) / duration - 1.f;
        float multiplier = Clamp(duration*.007f,2.f,4.f);
        score = 1.f - AbsFloat(score);
        float rank = (score*score*100.f);
        float noteHitPosX = m_isLeft ? sNoteHitPosLeftX : sNoteHitPosRightX;
        float noteHitPosY = m_isUp ? NOTE_RENDER_MULTI_UP_Y : NOTE_RENDER_MULTI_DOWN_Y;
        PlayParticleEffectForSingle(rank, Vec2(noteHitPosX, noteHitPosY), m_isLeft);
        g_theEvents->FireEvent(Stringf("AddScore rank=%f multi=%f", rank, multiplier), EVENT_GAME);
    }

    m_actualStart = 0;
    m_actualEnd = 0;
    g_theEvents->UnsubscribeObject(this);
}

//////////////////////////////////////////////////////////////////////////
void MultiNotes::Render(AABB2 const& bounds) const
{
    float minX = bounds.mins.x;
    float maxX = bounds.maxs.x;
    float multiRenderHalfSize = 6.f*NOTE_RENDER_HALF_SIZE;
    float halfXValue = (maxX - minX) * RENDER_HALF_FRACTION;
    sNoteHitPosLeftX = minX + halfXValue;
    sNoteHitPosRightX = maxX-halfXValue;
    Vec2 relativePos(0.f, multiRenderHalfSize);

    float baseYValue = m_isUp?NOTE_RENDER_MULTI_UP_Y:NOTE_RENDER_MULTI_DOWN_Y;

    float startAge = m_song->GetNoteAgeFromTimeMS(m_startMS);    
    startAge = ClampZeroToOne(startAge);
    float xStartPos = halfXValue * startAge;
    Vec2 startAnchor(xStartPos + minX, baseYValue);
    float rawEndAge = m_song->GetNoteAgeFromTimeMS(m_startMS+m_duration);
    float endAge = ClampZeroToOne(rawEndAge);
    float xEndPos = halfXValue*endAge;
    Vec2 endAnchor(xEndPos+minX, baseYValue-multiRenderHalfSize);
    AABB2 duration(endAnchor, startAnchor + relativePos);
    if (!m_isLeft) {    //right half
        startAnchor.x = maxX - xStartPos;
        endAnchor.x = maxX-xEndPos;
        Vec2 maxs = startAnchor+relativePos;
        duration = AABB2(maxs.x, endAnchor.y, endAnchor.x, maxs.y);
    }

    SpriteDefinition const& def = AssetManager::gAssetManager->m_multiMonsterAnim->GetSpriteDefAtTime(4.f*startAge);
    Vec2 uvMins, uvMaxs;
    def.GetUVs(uvMins, uvMaxs);
    if (m_isLeft) {
        SwapFloat(uvMins.x, uvMaxs.x);
    }

    Rgba8 drawColor = m_actualStart>0 ? Rgba8(150, 150, 150, 150) : Rgba8::RED;
    if (rawEndAge > 1.f) {
        drawColor = Lerp(Rgba8(0, 0, 0, 0), Rgba8::RED, (rawEndAge - 1.f) / NOTE_RENDER_FINISH_AGE);
        g_theRenderer->DrawSquare2D(startAnchor, multiRenderHalfSize * 2.f, drawColor, uvMins, uvMaxs);
        return;
    }

    Vec2 tailUVMins, tailUVMaxs;
    AssetManager::gAssetManager->m_monsterSheet->GetSpriteUVs(tailUVMins, tailUVMaxs, AssetManager::gAssetManager->m_monsterTailIndex);
    if (m_isLeft) {
        SwapFloat(tailUVMins.x, tailUVMaxs.x);
    }

    drawColor = m_actualStart>0?Rgba8(150,150,150,150):Rgba8::WHITE;
    drawColor = m_actualEnd>0?Rgba8(255,0,0,150):drawColor;
    g_theRenderer->DrawAABB2D(duration, Rgba8(255,255,255,180), tailUVMins, tailUVMaxs);
    g_theRenderer->DrawSquare2D(startAnchor, multiRenderHalfSize * 2.f, drawColor, uvMins, uvMaxs);
}

//////////////////////////////////////////////////////////////////////////
unsigned int MultiNotes::GetRenderBeginMS() const
{
    return m_startMS - NOTE_RENDER_MAX_TIME_MS;
}

//////////////////////////////////////////////////////////////////////////
unsigned int MultiNotes::GetRenderEndMS() const
{
    return m_duration + m_startMS;
}

//////////////////////////////////////////////////////////////////////////
bool MultiNotes::IsScored() const
{
    return m_actualStart>0;
}

//////////////////////////////////////////////////////////////////////////
bool MultiNotes::HandleJoystickMoved(EventArgs& args)
{
    if (m_actualEnd > 0) {
        return false;
    }
    
    bool isLeft = args.GetValue("isLeft", true);
    if (m_isLeft != isLeft) {
        return false;
    }

    float yValue = args.GetValue("yValue", 0.f);
    float yDirection = m_isUp?1.f:-1.f;
    if (AbsFloat(yValue) < INPUT_JOYSTICK_DEAD_Y) {
        if (m_actualStart > 0) {
            m_actualEnd = m_song->GetSongElapsedMS();
        }
        return false;
    }

    float elapsedMS = (float)m_song->GetSongElapsedMS();
    if ((float)m_startMS - elapsedMS > (float)NOTE_SCORE_DELTA_TIME_MS) {
        return false;
    }

    if (yDirection * yValue > 0.f) {   
        if(m_actualStart==0){
            m_actualStart = m_song->GetSongElapsedMS();
        }
    }
    else{
        if (m_actualStart > 0) {
            m_actualEnd = m_song->GetSongElapsedMS();
            m_emitter->StopAndClear();
        }
        else {
            return false;
        }
    }

    if (m_emitter == nullptr) {
        float noteHitPosX = m_isLeft ? sNoteHitPosLeftX:sNoteHitPosRightX;
        float noteHitPosY = m_isUp ? NOTE_RENDER_MULTI_UP_Y:NOTE_RENDER_MULTI_DOWN_Y;
        float maxAge = ((float)m_duration-(float)m_actualStart+(float)m_startMS)*.001f;
        m_emitter = PlayParticleEffectForMulti(Vec2(noteHitPosX, noteHitPosY), m_isLeft, maxAge);
    }
    return true;
}
