#pragma once
#include "Engine/Core/EngineCommon.hpp"

class App;
class RenderContext;
class InputSystem;
class RandomNumberGenerator;
class AudioSystem;
class BitmapFont;
class Game;

constexpr unsigned int COMBO_START_COUNT = 5;
constexpr unsigned int COMBO_SINGLE_RATE_COUNT = 5;
constexpr unsigned int NOTE_RENDER_MAX_TIME_MS = 3000;
constexpr unsigned int NOTE_SCORE_DELTA_TIME_MS = 500;
constexpr float NOTE_RENDER_FINISH_AGE = (float)NOTE_SCORE_DELTA_TIME_MS / (float)NOTE_RENDER_MAX_TIME_MS;
constexpr float NOTE_RENDER_ATTACK_START_AGE=1.f - NOTE_RENDER_FINISH_AGE;
constexpr float NOTE_RENDER_MULTI_DOWN_Y = -300.f;
constexpr float NOTE_RENDER_MULTI_UP_Y = -100.f;
constexpr float NOTE_RENDER_HALF_SIZE = 25.f;
constexpr float COMBO_MAX_RATE=5.f;
constexpr float COMBO_PERFECT_RANK = 85.f;
constexpr float COMBO_GOOD_RANK = 60.f;
constexpr float COMBO_FAIR_RANK = 35.f;
constexpr float RENDER_CENTER_FRACTION = .2f;
constexpr float RENDER_HALF_FRACTION = (1.f-RENDER_CENTER_FRACTION)*.5f;
constexpr float INPUT_JOYSTICK_DEAD_Y = .3f;
constexpr float INPUT_FLOAT_DELTA_CHANGE = .1f;
constexpr float FONT_DEFAULT_ASPECT = 1.f;
constexpr float FONT_DEFAULT_KERNING = .3f;

extern App* g_theApp;
extern Game* g_theGame;
extern RandomNumberGenerator* g_theRNG;
extern AudioSystem* g_theAudio;
extern RenderContext* g_theRenderer;
extern InputSystem* g_theInput;
extern BitmapFont* g_theFont;

extern bool g_isDebugDrawing;
extern float gMusicVolume;
extern float gSFXVolume;
extern float gNoteDelayDelta;

enum eXboxButtonID : int;

extern eXboxButtonID gConfirmButton;
extern eXboxButtonID gBackButton;
extern eXboxButtonID gPauseButton;

typedef size_t SoundID;

extern SoundID gButtonSFXID;