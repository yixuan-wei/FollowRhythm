#include "Game/GameCommon.hpp"
#include "Engine/Input/XboxController.hpp"

App* g_theApp = nullptr;
RenderContext* g_theRenderer = nullptr;
InputSystem* g_theInput = nullptr;
RandomNumberGenerator* g_theRNG = nullptr;
AudioSystem* g_theAudio = nullptr;
BitmapFont* g_theFont = nullptr;
Game* g_theGame = nullptr;

bool g_isDebugDrawing = false;
float gMusicVolume = 1.f;
float gSFXVolume = 1.f;
float gNoteDelayDelta = 0;

eXboxButtonID gConfirmButton=XBOX_BUTTON_ID_A;
eXboxButtonID gBackButton = XBOX_BUTTON_ID_BACK;
eXboxButtonID gPauseButton = XBOX_BUTTON_ID_START;

SoundID gButtonSFXID = 0;