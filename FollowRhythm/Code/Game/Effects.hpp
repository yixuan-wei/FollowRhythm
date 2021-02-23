#pragma once

struct Vec2;
struct Rgba8;
class Emitter2D;

void InitEffects();

Emitter2D* PlayParticleEffectForSingle(float rank, Vec2 const& pos, bool isLeft);
Emitter2D* PlayParticleEffectForMulti(Vec2 const& pos, bool isLeft, float maxAge);