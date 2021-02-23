#include "Game/SingleNote.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Song.hpp"
#include "Game/Game.hpp"
#include "Game/Effects.hpp"
#include "Game/AssetManager.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"

#include "Engine/Renderer/RenderContext.hpp"

static float sNoteRenderMaxTime = (float)NOTE_RENDER_MAX_TIME_MS*.001f;
static float sNoteHitPosY = 0.f;
static float sNoteHitPosLeftX = 0.f;
static float sNoteHitPosRightX = 0.f;

//////////////////////////////////////////////////////////////////////////
SingleNote::SingleNote(Song* song, bool isLeft, unsigned int startMS)
    : Note(song, NOTE_SINGLE, isLeft, startMS)
{
}

//////////////////////////////////////////////////////////////////////////
void SingleNote::StartToSee()
{
    g_theEvents->RegisterMethodEvent("ButtonPressed", this, &SingleNote::HandleButtonPressed, "LB/RB pressed", EVENT_GAME);
}

//////////////////////////////////////////////////////////////////////////
void SingleNote::EndToSee()
{
    m_isHit = false;
    g_theEvents->UnsubscribeObject(this);
}

//////////////////////////////////////////////////////////////////////////
void SingleNote::Render(AABB2 const& bounds) const
{    
    float rawAge = m_song->GetNoteAgeFromTimeMS(m_startMS);
    float minX = bounds.mins.x;
    float maxX = bounds.maxs.x;
    float age = ClampZeroToOne(rawAge);
    float halfXValue = (maxX - minX) * RENDER_HALF_FRACTION;
    float xPos = halfXValue * age;
    sNoteHitPosLeftX = minX+halfXValue;
    sNoteHitPosY = Interpolate(bounds.mins.y, bounds.maxs.y, .65f);
    Vec2 anchor(xPos + minX, sNoteHitPosY);
    if (!m_isLeft) {    //right half
        anchor.x = maxX - xPos;
        sNoteHitPosRightX = maxX-halfXValue;
    }    

    float renderFraction = 15.f*NOTE_RENDER_HALF_SIZE;    
    Vec2 uvMins, uvMaxs;
    if (rawAge < NOTE_RENDER_ATTACK_START_AGE) {    //fly
        SpriteDefinition const& def = AssetManager::gAssetManager->m_singleMonsterAnim->GetSpriteDefAtTime(rawAge * 5.f);
        def.GetUVs(uvMins, uvMaxs);
    }
    else if (rawAge < 1.f) {    //attack
        SpriteDefinition const& def = AssetManager::gAssetManager->m_singleAttackAnim->GetSpriteDefAtTime((rawAge- NOTE_RENDER_ATTACK_START_AGE)*sNoteRenderMaxTime);
        def.GetUVs(uvMins, uvMaxs);
    }
    else {  //finish
        SpriteDefinition const& def = AssetManager::gAssetManager->m_singleFinishAnim->GetSpriteDefAtTime((rawAge - 1.f)*sNoteRenderMaxTime*2.f);
        def.GetUVs(uvMins, uvMaxs);
    }
    if (!m_isLeft) {
        SwapFloat(uvMins.x, uvMaxs.x);
    }

    if(m_isHit){
        if (rawAge > 1.f) {
            return;
        }
        g_theRenderer->DrawSquare2D(anchor, renderFraction, Rgba8(150, 150, 150, 150),uvMins, uvMaxs);
        return;
    }
    
    Rgba8 drawColor = Rgba8::RED;
    if (rawAge > 1.f) {
        drawColor = Lerp(Rgba8(0,0,0,0), Rgba8::RED, (rawAge-1.f)/NOTE_RENDER_FINISH_AGE);
        g_theRenderer->DrawSquare2D(anchor, renderFraction, drawColor, uvMins, uvMaxs);
        return;
    }
    
    g_theRenderer->DrawSquare2D(anchor, renderFraction, Rgba8::WHITE, uvMins, uvMaxs);    
}

//////////////////////////////////////////////////////////////////////////
unsigned int SingleNote::GetRenderBeginMS() const
{
    if (m_startMS < NOTE_RENDER_MAX_TIME_MS) {
        return 0;
    }

    return m_startMS - NOTE_RENDER_MAX_TIME_MS;
}

//////////////////////////////////////////////////////////////////////////
unsigned int SingleNote::GetRenderEndMS() const
{
    return m_startMS + NOTE_SCORE_DELTA_TIME_MS;
}

//////////////////////////////////////////////////////////////////////////
bool SingleNote::IsScored() const
{
    return m_isHit;
}

//////////////////////////////////////////////////////////////////////////
bool SingleNote::HandleButtonPressed(EventArgs& args)
{
    if (m_isHit) {
        return false;
    }

    bool isLeft = args.GetValue("isLeft", true);
    if (m_isLeft == isLeft) {
        float elapsedMS = (float)m_song->GetSongElapsedMS();
        float delta = (elapsedMS - (float)m_startMS);
        float score = AbsFloat(delta - gNoteDelayDelta) / (float)NOTE_SCORE_DELTA_TIME_MS;
        if (score >= 1.f) {
            return false;
        }
        else {
            m_isHit = true;
            score = 1.f-score;
            float rank = (score*100.f);
            float noteHitPosX = m_isLeft? sNoteHitPosLeftX:sNoteHitPosRightX;
            PlayParticleEffectForSingle(rank, Vec2(noteHitPosX, sNoteHitPosY), m_isLeft);
            g_theEvents->FireEvent(Stringf("AddScore rank=%f delta=%f", rank, delta), EVENT_GAME);
            return true;
        }
    }
    else {
        return false;
    }
}
