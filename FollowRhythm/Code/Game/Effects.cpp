#include "Game/Effects.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Engine/Renderer/ParticleSystem2D.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/Emitter2D.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Delegate.hpp"
#include "Engine/Core/Clock.hpp"

static Texture* sParticleTex = nullptr;

//////////////////////////////////////////////////////////////////////////
static void TransformPartileForAge(float age, float maxAge, Vec2& scale, Rgba8& color)
{
    float deltaSeconds = (float)g_theGame->GetGameClock()->GetLastDeltaSeconds();
    float fraction = deltaSeconds/(maxAge-age+deltaSeconds);

    color.a = (unsigned char)Interpolate((float)color.a, 150.f, fraction);
    scale;
}

//////////////////////////////////////////////////////////////////////////
static Emitter2D* PlayParticleEffect(Vec2 const& pos, float dirDegrees, Rgba8 const& color, float maxAge,
    FloatRange const& speedRange)
{
    Emitter2D* result = ParticleSystem2D::gParticleSystem2D->StartEmitter(100, pos, Vec2::ZERO,
        dirDegrees, Vec2(15.f, 15.f), maxAge, Vec2::ZERO, color, nullptr, sParticleTex,
        FloatRange(0.f, 85.f), speedRange, FloatRange(.3f, 1.f),
        FloatRange(.6f, 1.f));
    result->onParticleGrowOlder.Subscribe(TransformPartileForAge);
    return result;
}

//////////////////////////////////////////////////////////////////////////
void InitEffects()
{
    ParticleSystem2D::gParticleSystem2D = new ParticleSystem2D(g_theRenderer);
    sParticleTex = g_theRenderer->CreateOrGetTextureFromFile("data/images/particle.png");
}

//////////////////////////////////////////////////////////////////////////
Emitter2D* PlayParticleEffectForSingle(float rank, Vec2 const& pos, bool isLeft)
{
    Rgba8 color = Rgba8::RED;
    if (rank >= COMBO_PERFECT_RANK){    color = Rgba8(255, 223, 0);  }
    else if(rank>=COMBO_GOOD_RANK){     color = Rgba8(51,153,255); }
    else if(rank>=COMBO_FAIR_RANK){     color = Rgba8::WHITE; }

    float dirDegrees=0.f;
    if(isLeft){  dirDegrees=180.f;  }

    return PlayParticleEffect(pos, dirDegrees, color, .5f,FloatRange(20.f,140.f));    
}

//////////////////////////////////////////////////////////////////////////
Emitter2D* PlayParticleEffectForMulti(Vec2 const& pos, bool isLeft, float maxAge)
{
    float dirDegrees = 0.f;
    if (isLeft) { dirDegrees = 180.f; }

    return PlayParticleEffect(pos, dirDegrees, Rgba8::WHITE, maxAge, FloatRange(10.f,40.f));
}

