#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Game/ButtonList.hpp"
#include "Game/Song.hpp"
#include "Game/SongManager.hpp"
#include "Game/CircleButtonList.hpp"
#include "Game/AssetManager.hpp"
#include "Game/Effects.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/ParticleSystem2D.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/MathUtils.hpp"

static SoundPlaybackID sAttractPlayID;

static ButtonList sMainMenuButtons;
static ButtonList sSettingsButtons;
static ButtonList sConfirmButtons;
static CircleButtonList sMusicSelectButtons;
static Timer sClearHistoryTimer;
static Timer sSFXPlayTimer;
static Timer sSongInvalidTimer;

static Background sMenuBackground;
static Rgba8 sMenuBGTint = Rgba8(150,150,150);

static const char* sConfigFilePath = "data/log/config.txt";

enum sMainMenuItem
{
	MAIN_MENU_START = 0,
	MAIN_MENU_TUTORIAL,
	MAIN_MENU_SETTINGS,
	MAIN_MENU_CREDITS,
	MAIN_MENU_QUIT
};
enum sSettingsItem
{
	SETTINGS_MUSIC_VOL = 0,
	SETTINGS_SFX_VOL,
	SETTINGS_CALIBRATION,
	SETTINGS_CLEAR_HISTORY,
	SETTINGS_BACK
};

//////////////////////////////////////////////////////////////////////////
COMMAND(UpdateBackground, "randomly update menu background", eEventFlag::EVENT_GAME)
{
	UNUSED(args);
	sMenuBackground = AssetManager::gAssetManager->GetRandomBackgroundPaths();
	return true;
}

//////////////////////////////////////////////////////////////////////////
static void InitConfigData()
{
	Strings calibTexts = FileReadLines(sConfigFilePath);
	if (calibTexts.size() != 3 ) {
		return;
	}
	Strings delayTexts = SplitStringOnDelimiter(calibTexts[0], ':');
	if(delayTexts.size()==2){
		gNoteDelayDelta = StringConvert(delayTexts[1].c_str(), 0.f);
	}
	Strings musicTexts = SplitStringOnDelimiter(calibTexts[1], ':');
	if(musicTexts.size()==2){
		gMusicVolume = StringConvert(musicTexts[1].c_str(),1.f);
	}
	Strings sfxTexts = SplitStringOnDelimiter(calibTexts[2], ':');
	if(sfxTexts.size()==2){
		gSFXVolume = StringConvert(sfxTexts[1].c_str(), 1.f);
	}
}

static eXboxButtonID GetXboxButtonIDFromText(std::string const& text)
{
	if (text == "A") {		return XBOX_BUTTON_ID_A;	}
	else if(text=="B"){		return XBOX_BUTTON_ID_B;	}
	else if(text=="X"){		return XBOX_BUTTON_ID_X;	}
	else if(text=="Y"){		return XBOX_BUTTON_ID_Y;	}
	else if(text=="LB"){	return XBOX_BUTTON_ID_LSHOULDER;}
	else if(text=="RB"){	return XBOX_BUTTON_ID_RSHOULDER;}
	else if(text=="Start"){ return XBOX_BUTTON_ID_START;}
	else if(text=="Back"){  return XBOX_BUTTON_ID_BACK; }
	else if(text=="DpadUp"){	return XBOX_BUTTON_ID_DPAD_UP;}
	else if(text=="DpadDown"){	return XBOX_BUTTON_ID_DPAD_DOWN;}
	else if(text=="DpadLeft"){  return XBOX_BUTTON_ID_DPAD_LEFT;}
	else if(text=="DpadRight"){ return XBOX_BUTTON_ID_DPAD_RIGHT;}
	else { return XBOX_BUTTON_ID_A;}
}

//////////////////////////////////////////////////////////////////////////
static void InitButtonMappings()
{
	std::string confirmText = g_gameConfigBlackboard->GetValue("confirmButton", "A");
	gConfirmButton = GetXboxButtonIDFromText(confirmText);
	std::string backText = g_gameConfigBlackboard->GetValue("backButton","Back");
	gBackButton = GetXboxButtonIDFromText(backText);
	std::string pauseText = g_gameConfigBlackboard->GetValue("pauseButton", "Start");
	gPauseButton = GetXboxButtonIDFromText(pauseText);
}

//////////////////////////////////////////////////////////////////////////
static void SaveConfigDelta() 
{
	std::string calibText = Stringf("Config Delay:%f\nMusic Vol:%f\nSFX Vol:%f",gNoteDelayDelta, gMusicVolume, gSFXVolume);
	if (!FileWriteToDisk(sConfigFilePath, calibText.data(), calibText.size())) {
		ERROR_RECOVERABLE("Fail to save calibration delta to disk");
	}
}

//////////////////////////////////////////////////////////////////////////
static void UpdateMenuForMusicVolume() 
{
	gMusicVolume = Clamp(gMusicVolume,0.f,2.0f);
    int displayVol = (int)(gMusicVolume * 50.f);
    sSettingsButtons.m_buttons[(int)(SETTINGS_MUSIC_VOL)].text = Stringf("Music Volume: %i", displayVol);

	g_theAudio->SetSoundPlaybackVolume(sAttractPlayID, gMusicVolume);
}

//////////////////////////////////////////////////////////////////////////
static void UpdateMenuForSFXVolume()
{
	gSFXVolume = Clamp(gSFXVolume, 0.f, 2.0f);
	if(!sSFXPlayTimer.IsRunning() || sSFXPlayTimer.HasElapsed()){
		g_theAudio->PlaySound(gButtonSFXID, false, gSFXVolume);
		sSFXPlayTimer.SetTimerSeconds(0.3);
	}
    int displayVol = (int)(gSFXVolume * 50.f);
    sSettingsButtons.m_buttons[(int)(SETTINGS_SFX_VOL)].text = Stringf("SFX Volume: %i", displayVol);
}

//////////////////////////////////////////////////////////////////////////
void GetMenuButtonsInfoFromBounds(AABB2 const& bounds, unsigned int buttonNum,
	AABB2& singleBounds, Vec2& singleTrans)
{
	GUARANTEE_OR_DIE(buttonNum>0, "buttonNum in GetMenuButtonsInfoFromBounds == 0");
	float bNum = (float)buttonNum;

    Vec2 totalDim = bounds.GetDimensions();
    float singleHeight = totalDim.y / (bNum+1.f);
    float gapHeight = (1.f/(bNum-1.f)) * singleHeight;
    Vec2 singleDim(totalDim.x, singleHeight);
    singleBounds = AABB2(-singleDim, Vec2::ZERO);
    singleBounds.Translate(bounds.maxs);
    singleTrans = Vec2(0.f, -singleHeight - gapHeight);
}

//////////////////////////////////////////////////////////////////////////
static void InitMainMenuButtons(AABB2 const& bounds)
{
	AABB2 singleBound;
	Vec2 singleTrans;
	GetMenuButtonsInfoFromBounds(bounds, 5, singleBound, singleTrans);

	sMainMenuButtons.m_buttons.push_back(Button("Start", true, singleBound));	
	singleBound.Translate(singleTrans);
	sMainMenuButtons.m_buttons.push_back(Button("Controls", false, singleBound));
    singleBound.Translate(singleTrans);
    sMainMenuButtons.m_buttons.push_back(Button("Settings", false, singleBound)); 
    singleBound.Translate(singleTrans);
    sMainMenuButtons.m_buttons.push_back(Button("Credits", false, singleBound));    
    singleBound.Translate(singleTrans);
	sMainMenuButtons.m_buttons.push_back(Button("Quit", false, singleBound));

	sMainMenuButtons.m_buttonTex=g_theRenderer->CreateOrGetTextureFromFile("data/images/buttons-2d/6-new.png");
}

//////////////////////////////////////////////////////////////////////////
static void InitConfirmButton(AABB2 const& bounds)
{
	sConfirmButtons.m_buttons.push_back(Button("Back", true, bounds));

	sConfirmButtons.m_buttonTex = g_theRenderer->CreateOrGetTextureFromFile("data/images/buttons-2d/6-new.png");
}

//////////////////////////////////////////////////////////////////////////
static void InitSettingsMenuButtons(AABB2 const& bounds)
{
	AABB2 singleBound;
	Vec2 singleTrans;
	GetMenuButtonsInfoFromBounds(bounds, 5, singleBound, singleTrans);

	sSettingsButtons.m_buttons.push_back(Button("Music Volume: 50", true, singleBound));
	singleBound.Translate(singleTrans);
    sSettingsButtons.m_buttons.push_back(Button("SFX Volume: 50", false, singleBound));
    singleBound.Translate(singleTrans);
    sSettingsButtons.m_buttons.push_back(Button("Calibration", false, singleBound));
    singleBound.Translate(singleTrans);
    sSettingsButtons.m_buttons.push_back(Button("Clear History", false, singleBound));
    singleBound.Translate(singleTrans);
    sSettingsButtons.m_buttons.push_back(Button("Back", false, singleBound));

	sSettingsButtons.m_buttonTex = g_theRenderer->CreateOrGetTextureFromFile("data/images/buttons-2d/6-new.png");

	UpdateMenuForSFXVolume();
	UpdateMenuForMusicVolume();
}

//////////////////////////////////////////////////////////////////////////
Game::Game()
{
}

//////////////////////////////////////////////////////////////////////////
void Game::StartUp()
{
	//Init
	g_theRNG = new RandomNumberGenerator();
	g_theGame = this;
	g_theInput->PushMouseOptions(eMousePositionMode::MOUSE_ABSOLUTE, false, false);
	m_gameClock = new Clock();
	g_theRenderer->SetupParentClock(m_gameClock);

	g_theFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/InnerSpeaker");
	m_RNG = new RandomNumberGenerator();
	m_worldCamera = new Camera();
	m_uiCamera = new Camera();

	//init camera settings
	Vec2 resolution = g_theApp->GetWindowDimensions();
	Vec2 halfSize = resolution*.5f;
	m_worldCamera->SetClearMode(CLEAR_COLOR_BIT, Rgba8::BLACK);
	m_worldCamera->SetOrthoView(-halfSize, halfSize);
	
	m_uiCamera->SetColorTarget(nullptr);
	m_uiCamera->SetOrthoView(-halfSize, halfSize );
}

//////////////////////////////////////////////////////////////////////////
void Game::ShutDown()
{	
	SaveConfigDelta();
	g_theEvents->UnsubscribeObject(this);
	g_theInput->PopMouseOptions();

	delete ParticleSystem2D::gParticleSystem2D;
	delete m_songManager;
    delete m_worldCamera;
	delete m_uiCamera;
    delete m_RNG;

}

//////////////////////////////////////////////////////////////////////////
void Game::Update()
{	
	if (!m_startedLoading) {	//first frame
		m_startedLoading = true;
	}
	else if(m_isLoading){	//loading start
		LoadAndInitialize();
	}
	else{	//load finished
        UpdateForInput();
        if (m_state == GAME_MUSIC_PLAY || m_state == GAME_SETTINGS_CALIBRATE) {
            m_songManager->Update();
        }
        ParticleSystem2D::gParticleSystem2D->Update();
	}
}

//////////////////////////////////////////////////////////////////////////
void Game::Render() const
{
	RenderForGame();

    RenderForUI();

	DebugRenderWorldToCamera(m_worldCamera);
}

//////////////////////////////////////////////////////////////////////////
void Game::StartOfSong()
{
	m_state = GAME_MUSIC_PLAY;
}

//////////////////////////////////////////////////////////////////////////
void Game::EndOfSong()
{
	if (m_state != GAME_MUSIC_PLAY) {
		return;
	}

	m_state = GAME_MUSIC_SELECT;
}

//////////////////////////////////////////////////////////////////////////
void Game::LoadAndInitialize()
{
    //init songs   
    std::string assetPath = g_gameConfigBlackboard->GetValue("assetsReading", "data/assets.xml");
    AssetManager::gAssetManager = new AssetManager(assetPath.c_str());
    sMenuBackground = AssetManager::gAssetManager->GetRandomBackgroundPaths();
	InitButtonAssets();

	m_songManager = new SongManager(this, "data/music/");

	//g_theRenderer->CreateOrGetTextureFromFile("data/images/base.png");
	g_theRenderer->CreateOrGetTextureFromFile("data/images/gamepad.png");
	g_theRenderer->CreateOrGetTextureFromFile("data/images/buttons-2d/progress.png");
	g_theRenderer->CreateOrGetTextureFromFile("data/images/minus.png");
	g_theRenderer->CreateOrGetTextureFromFile("data/images/plus.png");
	g_theRenderer->CreateOrGetTextureFromFile("data/images/base.png");

    //config
    InitConfigData();

	//menus
	InitMusicSelectMenu();

	AABB2 uiBounds = m_uiCamera->GetBounds();
	AABB2 mainMenuButtonBounds = uiBounds.GetBoxAtBottom(.666f);
	mainMenuButtonBounds.ChopBoxOffLeft(.4f);
	mainMenuButtonBounds.ChopBoxOffRight(.6667f);
	mainMenuButtonBounds.ChopBoxOffBottom(.1f);
	InitMainMenuButtons(mainMenuButtonBounds);

	AABB2 settingsBounds = uiBounds.GetBoxAtBottom(.5f);
	settingsBounds.ChopBoxOffBottom(.1f);
	settingsBounds.ChopBoxOffLeft(.35f);
	settingsBounds.ChopBoxOffRight(7.f/13.f);
	InitSettingsMenuButtons(settingsBounds);

	AABB2 confirmBounds = uiBounds.GetBoxAtBottom(.2f);
	confirmBounds.ChopBoxOffBottom(.5f);
	confirmBounds.ChopBoxOffLeft(.4f);
	confirmBounds.ChopBoxOffRight(.667f);
	InitConfirmButton(confirmBounds);

	//button mappings
	InitButtonMappings();

	//effects
	InitEffects();

	//attract music play
	SoundID attractMusic = g_theAudio->CreateOrGetSound("data/music/toedit/Dread_p_-_16_-_War_Criminal.mp3");
	sAttractPlayID = g_theAudio->PlaySound(attractMusic, true);
	
	m_isLoading = false;
	TODO("Eliminate all console errors before final");
	g_theConsole->SetIsOpen(false);
}

//////////////////////////////////////////////////////////////////////////
void Game::InitMusicSelectMenu()
{
	m_songManager->InitMusicSelectMenu(&sMusicSelectButtons);

	AABB2 bounds = m_uiCamera->GetBounds();
	bounds.ChopBoxOffTop(.3f);
	bounds.ChopBoxOffRight(.5f);
	bounds.ChopBoxOffLeft(.15f);
	bounds.ChopBoxOffBottom(.5f);
	
	sMusicSelectButtons.m_generalDrawBounds = bounds;
	sMusicSelectButtons.m_singleBoundDim = bounds.GetDimensions();
	sMusicSelectButtons.m_greyColor = Rgba8(100,100,10);
	sMusicSelectButtons.m_highlightColor = Rgba8::WHITE;

	sMusicSelectButtons.m_buttonTex = g_theRenderer->CreateOrGetTextureFromFile("data/images/buttons-2d/7.png");
}

//////////////////////////////////////////////////////////////////////////
void Game::UpdateForInput()
{
	if (g_theConsole->IsOpen() || m_isLoading) {
		return;
	}

    XboxController const& controller = g_theInput->GetXboxController(0);
    if (!controller.IsConnected() || m_state==GAME_MUSIC_PLAY) {
        return;
    }

	//back to previous
	if (controller.GetButtonState(gBackButton).WasJustPressed()) {
		switch (m_state) {
        case GAME_ATTRACT: {
            g_theApp->HandleQuitRequisted();
            return;
        }
		case GAME_MAIN_MENU:		{
			m_state = GAME_ATTRACT;
			g_theAudio->SetSoundPaused(sAttractPlayID, false);
			break;
		}
		case GAME_TUTORIAL:
		case GAME_SETTINGS:
		case GAME_CREDITS:
		case GAME_MUSIC_SELECT:		{
			m_state = GAME_MAIN_MENU;
			g_theEvents->FireEvent("UpdateBackground", eEventFlag::EVENT_GAME );
			break;
		}		
		case GAME_SETTINGS_CALIBRATE:		{
			m_state = GAME_SETTINGS;
			m_songManager->StopCalibration();
			break;
		}
		}
		g_theAudio->PlaySound(gButtonSFXID, false, gSFXVolume);
	}

	//progress to next
	if (controller.GetButtonState(gConfirmButton).WasJustPressed()) {
		switch (m_state) {
		case GAME_ATTRACT:		{
			m_state = GAME_MAIN_MENU;
			g_theEvents->FireEvent("UpdateBackground", eEventFlag::EVENT_GAME);
			g_theAudio->SetSoundPaused(sAttractPlayID, true);
			break;
		}
		case GAME_MAIN_MENU:		{
			sMainMenuItem curItem = (sMainMenuItem)sMainMenuButtons.m_selectedIndex;
			switch (curItem) {
				case MAIN_MENU_START:		m_state = GAME_MUSIC_SELECT; break;
				case MAIN_MENU_TUTORIAL:	m_state = GAME_TUTORIAL; break;
				case MAIN_MENU_SETTINGS:	m_state = GAME_SETTINGS; break;
				case MAIN_MENU_CREDITS:		m_state = GAME_CREDITS; break;
				case MAIN_MENU_QUIT:		g_theApp->HandleQuitRequisted(); return;
			}
			sMainMenuButtons.m_selectedIndex = 0;
			break;
		}
		case GAME_MUSIC_SELECT:		{
			if(m_songManager->StartPlaySong(sMusicSelectButtons.m_selectedIndex)){
				m_state = GAME_MUSIC_PLAY;
			}
			else {
				sSongInvalidTimer.SetTimerSeconds(m_gameClock,1.0);
			}
			break;
		}
		case GAME_SETTINGS:		{
			sSettingsItem curItem = (sSettingsItem)sSettingsButtons.m_selectedIndex;
			switch (curItem)
			{
			case SETTINGS_CALIBRATION:{
				m_state = GAME_SETTINGS_CALIBRATE;
				m_songManager->StartCalibration();
				break;
			}
			case SETTINGS_CLEAR_HISTORY:	{
				m_songManager->ClearScoreHistory();
				sClearHistoryTimer.SetTimerSeconds(m_gameClock, 1.0);
				break;
			}
			case SETTINGS_BACK:{
				m_state = GAME_MAIN_MENU;
				g_theEvents->FireEvent("UpdateBackground", eEventFlag::EVENT_GAME);
				sSettingsButtons.m_selectedIndex = 0;
				break;
			}
			}
			break;
		}
		case GAME_SETTINGS_CALIBRATE:		{
			m_state = GAME_SETTINGS;
			gNoteDelayDelta = Song::GetAverageCalibrationDeltaTime();
			m_songManager->StopCalibration();
			break;
		}
		case GAME_CREDITS:
		case GAME_TUTORIAL:		{
			m_state = GAME_MAIN_MENU;
			g_theEvents->FireEvent("UpdateBackground", eEventFlag::EVENT_GAME);
			break;
		}
		}
		g_theAudio->PlaySound(gButtonSFXID, false, gSFXVolume);
	}

	float deltaTime = (float)m_gameClock->GetLastDeltaSeconds();
	float deltaFloatChange = deltaTime * INPUT_FLOAT_DELTA_CHANGE;

	//smaller the number
	if (controller.GetButtonState(XBOX_BUTTON_ID_LSHOULDER).IsPressed()) {
		switch (m_state)
		{
		case GAME_SETTINGS:		{
			sSettingsItem curItem = (sSettingsItem)sSettingsButtons.m_selectedIndex;
			switch (curItem) {
			case SETTINGS_MUSIC_VOL:			{
				gMusicVolume -= deltaFloatChange;
				UpdateMenuForMusicVolume();
				break;
			}
            case SETTINGS_SFX_VOL:            {
                gSFXVolume -= deltaFloatChange;				
                UpdateMenuForSFXVolume();
                break;
            }
			}
			break;
		}
		}
	}

	//larger the number
	if (controller.GetButtonState(XBOX_BUTTON_ID_RSHOULDER).IsPressed()) {
		switch (m_state)
		{
		case GAME_SETTINGS:{
			sSettingsItem curItem = (sSettingsItem)sSettingsButtons.m_selectedIndex;
            switch (curItem) {
            case SETTINGS_MUSIC_VOL:            {
                gMusicVolume += deltaFloatChange;
                UpdateMenuForMusicVolume();
                break;
            }
            case SETTINGS_SFX_VOL:            {
                gSFXVolume += deltaFloatChange;
                UpdateMenuForSFXVolume();
                break;
            }
            }
			break;
		}
		}
	}

	//UI navigation
	if (m_state == GAME_MAIN_MENU) {
		sMainMenuButtons.UpdateForNavigationInput(controller);
	}
	else if (m_state == GAME_SETTINGS) {
		sSettingsButtons.UpdateForNavigationInput(controller);
		sSettingsItem curItem = (sSettingsItem)sSettingsButtons.m_selectedIndex;
		if (curItem == SETTINGS_MUSIC_VOL) {
			g_theAudio->SetSoundPaused(sAttractPlayID, false);
		}
		else {
			g_theAudio->SetSoundPaused(sAttractPlayID, true);
		}
	}
	else if (m_state == GAME_MUSIC_SELECT) {
		sMusicSelectButtons.UpdateForNavigationInput(controller);
	}
}

//////////////////////////////////////////////////////////////////////////
void Game::RenderForGame() const
{
    g_theRenderer->BeginCamera(m_worldCamera);
    g_theRenderer->DisableDepth();

	//TODO
	if(m_state==GAME_MUSIC_PLAY ){
		m_songManager->Render(m_worldCamera->GetBounds());
		ParticleSystem2D::gParticleSystem2D->Render();
	}

    g_theRenderer->EndCamera(m_worldCamera);
}

//////////////////////////////////////////////////////////////////////////
void Game::RenderForUI() const
{
    g_theRenderer->BeginCamera(m_uiCamera);
    g_theRenderer->DisableDepth();

	AABB2 uiBound = m_uiCamera->GetBounds();
	Vec2 uiDim = uiBound.GetDimensions();

	//TODO
	std::vector<Vertex_PCU> textVerts;
	if (m_isLoading) {
		g_theFont->AddVertsForTextInBox2D(textVerts, uiBound,uiDim.y*.1f, "Loading...");
	}
	else if(m_state==GAME_ATTRACT){
        g_theRenderer->BindDiffuseTexture(sMenuBackground.mainBG.c_str());
        g_theRenderer->DrawAABB2D(uiBound,sMenuBGTint);
		g_theFont->AddVertsForTextInBox2D(textVerts, uiBound, uiDim.y*.1f, "Follow Rhythm", Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .05f, FONT_DEFAULT_KERNING);
		g_theFont->AddVertsForTextInBox2D(textVerts, uiBound, uiDim.y*.02f, "[A] to start, [B] to quit", Rgba8(200,200,200), FONT_DEFAULT_ASPECT, Vec2(.5f,.2f), .05f, FONT_DEFAULT_KERNING);
	}
	else if (m_state == GAME_MAIN_MENU) {
		g_theRenderer->BindDiffuseTexture(sMenuBackground.mainBG.c_str());
		g_theRenderer->DrawAABB2D(uiBound, sMenuBGTint);
		AABB2 titleBound = uiBound.GetBoxAtTop(.334f);
		g_theFont->AddVertsForTextInBox2D(textVerts, titleBound, uiDim.y*.1f, "Follow Rhythm", Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .05f, FONT_DEFAULT_KERNING);
		sMainMenuButtons.Render(textVerts);
	}
	else if (m_state == GAME_MUSIC_SELECT) {
        g_theRenderer->BindDiffuseTexture(sMenuBackground.mainBG.c_str());
        g_theRenderer->DrawAABB2D(uiBound, sMenuBGTint);

		g_theFont->AddVertsForTextInBox2D(textVerts, uiBound.GetBoxAtTop(.2f), uiDim.y*.08f, "Music Select", Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .05f, FONT_DEFAULT_KERNING);
		g_theFont->AddVertsForTextInBox2D(textVerts, uiBound.GetBoxAtBottom(.25f), uiDim.y*.02f, "[A] to select   [B] to quit", Rgba8(200,200,200),FONT_DEFAULT_ASPECT,Vec2(.1f, .5f), .05f, FONT_DEFAULT_KERNING);
		sMusicSelectButtons.Render(textVerts);

		//music info render
		AABB2 InfoBounds = uiBound.GetBoxAtRight(.5f);
		InfoBounds.ChopBoxOffTop(.25f);
		InfoBounds.ChopBoxOffBottom(.1f);
		InfoBounds.ChopBoxOffRight(.1f);
		AABB2 imageBounds = InfoBounds.GetBoxAtTop(.6f);
		Vec2 imageDim = imageBounds.GetDimensions();
		float imageLength = imageDim.x>imageDim.y?imageDim.y:imageDim.x;
		imageBounds.SetDimensions(Vec2(imageLength, imageLength));
		unsigned int selectedIndex = sMusicSelectButtons.m_selectedIndex;
		g_theRenderer->BindDiffuseTexture(m_songManager->GetSelectedSongImage(selectedIndex));
		g_theRenderer->DrawAABB2D(imageBounds);

		AABB2 textBounds = InfoBounds.GetBoxAtBottom(.4f);
		textBounds.ChopBoxOffLeft(.24f);
		g_theFont->AddVertsForTextInBox2D(textVerts, textBounds, uiDim.y*.026f, 
			m_songManager->GetSelectedSongInfo(selectedIndex), Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_CENTER_LEFT, .05f, FONT_DEFAULT_KERNING);

		if (sSongInvalidTimer.IsRunning() && !sSongInvalidTimer.HasElapsed()) {
            AABB2 popOut = uiBound;
            popOut.SetDimensions(.3f * uiDim);
            g_theFont->AddVertsForTextInBox2D(textVerts, popOut, uiDim.y * .02f, "Song Invalid!", Rgba8::BLACK, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .05f, FONT_DEFAULT_KERNING);
            g_theRenderer->BindDiffuseTexture((Texture*)nullptr);
            g_theRenderer->DrawAABB2D(popOut, Rgba8(255, 255, 255, 150));
		}
	}
	else if (m_state == GAME_SETTINGS || m_state==GAME_SETTINGS_CALIBRATE) {
        g_theRenderer->BindDiffuseTexture(sMenuBackground.mainBG.c_str());
        g_theRenderer->DrawAABB2D(uiBound, sMenuBGTint);
		AABB2 titleBound = uiBound.GetBoxAtTop(.2f);
		g_theFont->AddVertsForTextInBox2D(textVerts, titleBound, uiDim.y*.08f, "Settings", Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .05f, FONT_DEFAULT_KERNING);

		sSettingsItem curItem = (sSettingsItem)sSettingsButtons.m_selectedIndex;
		if (curItem == SETTINGS_MUSIC_VOL || curItem == SETTINGS_SFX_VOL) {
			AABB2 buttonBounds = sSettingsButtons.m_buttons[sSettingsButtons.m_selectedIndex].drawBounds;
			buttonBounds.SetDimensions(buttonBounds.GetDimensions()*sSettingsButtons.m_highlightScale);
			Vec2 buttonDim = buttonBounds.GetDimensions();

			AABB2 leftBounds(uiBound.mins.x, buttonBounds.mins.y, buttonBounds.mins.x, buttonBounds.maxs.y);
			AABB2 leftIconBound = leftBounds.ChopBoxOffRight(0.f, buttonDim.y);
			leftIconBound.SetDimensions(.8f*leftIconBound.GetDimensions());
			g_theRenderer->BindDiffuseTexture("data/images/minus.png");
			g_theRenderer->DrawAABB2D(leftIconBound);
			g_theFont->AddVertsForTextInBox2D(textVerts, leftBounds,buttonDim.y*.6f, "LB", Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_CENTER_RIGHT, .05f, FONT_DEFAULT_KERNING);

			AABB2 rightBounds(buttonBounds.maxs.x, buttonBounds.mins.y, uiBound.maxs.x, buttonBounds.maxs.y);
			AABB2 rightIconBound = rightBounds.ChopBoxOffLeft(0.f, buttonDim.y);
			rightIconBound.SetDimensions(.8f*rightIconBound.GetDimensions());
			g_theRenderer->BindDiffuseTexture("data/images/plus.png");
			g_theRenderer->DrawAABB2D(rightIconBound);
			g_theFont->AddVertsForTextInBox2D(textVerts, rightBounds, buttonDim.y*.6f, "RB", Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_CENTER_LEFT, .05f, FONT_DEFAULT_KERNING);
		}

		sSettingsButtons.Render(textVerts);
		
		if (m_state == GAME_SETTINGS_CALIBRATE) {
			g_theRenderer->BindDiffuseTexture((Texture*)nullptr);
			g_theRenderer->DrawAABB2D(uiBound, Rgba8(0,0,0,100));

			AABB2 textBound = uiBound.GetBoxAtTop(.45f);
			textBound.ChopBoxOffTop(.5f);
			textBound.ChopBoxOffLeft(.65f);
			textBound.ChopBoxOffRight(.2f);
			std::string calibText = Stringf("Previous Delta: %.0f ms\nCurrent Delta: %.0f ms\n\n", 
				gNoteDelayDelta, Song::GetAverageCalibrationDeltaTime());
			calibText += "[LB] Hit Note\n[A] Confirm Calibration\n[B] Cancel Calibration";
			float textHeight = textBound.GetDimensions().y*.1f;
			g_theFont->AddVertsForTextInBox2D(textVerts, textBound, textHeight, calibText, Rgba8::WHITE, FONT_DEFAULT_ASPECT,ALIGN_CENTERED, .1f, FONT_DEFAULT_KERNING);
		}

		if (sClearHistoryTimer.IsRunning() && !sClearHistoryTimer.HasElapsed()) {
            AABB2 popOut = uiBound;
			popOut.SetDimensions(.3f * uiDim);
            g_theFont->AddVertsForTextInBox2D(textVerts, popOut, uiDim.y * .02f, "History Cleared!", Rgba8::BLACK, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .05f, FONT_DEFAULT_KERNING);
			g_theRenderer->BindDiffuseTexture((Texture*)nullptr);
            g_theRenderer->DrawAABB2D(popOut, Rgba8(255, 255, 255, 150));
		}
	}
	else if (m_state == GAME_TUTORIAL) {
        g_theRenderer->BindDiffuseTexture(sMenuBackground.mainBG.c_str());
        g_theRenderer->DrawAABB2D(uiBound, sMenuBGTint);

		AABB2 titleBound = uiBound.GetBoxAtTop(.2f);
		g_theFont->AddVertsForTextInBox2D(textVerts, titleBound, uiDim.y*.08f, "Controls", Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .05f, FONT_DEFAULT_KERNING);
		sConfirmButtons.Render(textVerts);

		AABB2 tutBound = uiBound.GetBoxAtBottom(.75f);
		tutBound.ChopBoxOffBottom(.25f);
		tutBound.ChopBoxOffLeft(.1f);
		tutBound.ChopBoxOffRight(1.f/9.f);

		AABB2 gamepadBound = tutBound;
		Vec2 dim = tutBound.GetDimensions();
		gamepadBound.SetDimensions(Vec2(dim.y, dim.y));
		g_theRenderer->BindDiffuseTexture("data/images/gamepad.png");
		g_theRenderer->DrawAABB2D(gamepadBound);

		Vec2 textDim((dim.x-dim.y), dim.y);
		Vec2 topLeft( gamepadBound.mins.x, gamepadBound.maxs.y);
		AABB2 leftTextBound(topLeft-textDim, topLeft);
		float textSize = textDim.y * .05f;
		g_theFont->AddVertsForTextInBox2D(textVerts, leftTextBound, textSize, 
			"Quit to previous [B]\n\n[LB]\nHit single note from left \n\n[Left Stick]\nMove up/down \nfor consecutive note \n from left ",
			Rgba8::WHITE, FONT_DEFAULT_ASPECT, Vec2(1.f, .5f), .1f, FONT_DEFAULT_KERNING);

		Vec2 bottomRight(gamepadBound.maxs.x, gamepadBound.mins.y);
		AABB2 rightTextBound(bottomRight, bottomRight+textDim);
        g_theFont->AddVertsForTextInBox2D(textVerts, rightTextBound, textSize,
            "[A] Confirm Selection\n\n[RB]\n Hit single note from right\n\n[Right Stick]\n Move up/down\n for consecutive note\n from right",
            Rgba8::WHITE, FONT_DEFAULT_ASPECT, Vec2(0.f, .5f), .1f,FONT_DEFAULT_KERNING);
	}
	else if (m_state == GAME_CREDITS) {
		g_theRenderer->BindDiffuseTexture(sMenuBackground.mainBG.c_str());
		g_theRenderer->DrawAABB2D(uiBound, sMenuBGTint);

		g_theFont->AddVertsForTextInBox2D(textVerts, uiBound.GetBoxAtTop(.15f), uiDim.y*.06f, "Credits", 
			Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .05f, FONT_DEFAULT_KERNING);
		sConfirmButtons.Render(textVerts);

		AABB2 textBounds = uiBound.GetBoxAtBottom(.85f);
		textBounds.ChopBoxOffLeft(.12f);
		textBounds.ChopBoxOffBottom(.23f);
		std::string text = "Pictures:\n\
Buttons/Progress Bar: https://craftpix.net/freebies/free-buttons-2d-game-objects/\n\
Xbox gamepad:         https://favpng.com/png_view/xbox-transparent-xbox-360-controller-xbox-one-controller-joystick-\n\
                          clip-art-png/4QnXALva\n\
+/- icon:             https://www.iconfont.cn/collections/detail?cid=11253\n\
Backgrounds:          https://craftpix.net/freebies/free-post-apocalyptic-pixel-art-game-backgrounds/\n\
\nAnimations: \n\
Red Monster:          https://lionheart963.itch.io/flying-eye-creature\n\
Grey Monster:         https://luizmelo.itch.io/monsters-creatures-fantasy\n\
\nFont:                 https://www.dafont.com/new-innerspeaker.font\n\
\nMusic (with album picture):\n\
Merry Christmas:      https://freemusicarchive.org/music/Borrtex/Christmas_Time_II/12_Borrtex_-_Merry_Christmas_1\n\
Burning Soul:         https://freemusicarchive.org/music/Black_Bones/Pirates_of_the_Coast/04_-_Black_Bones_-_Burning_Soul\n\
Future Vision:        https://freemusicarchive.org/music/Dread_Pirate_Roberts/Word_Power/Future_Vision\n\
Your Name:            https://freemusicarchive.org/music/Vincent_Augustus/Your_Name_1208/your_name_1454\n\
Background Song:      https://freemusicarchive.org/music/Dread_Pirate_Roberts/Word_Power/War_Criminal\n\
Calibration Song:     https://freemusicarchive.org/music/Blue_Dot_Sessions/RadioPink/Vienna_Beat\n\
Button Scroll:        https://freesound.org/s/485486/\n\
Button Click:         https://freesound.org/s/478196/\n\
";
        g_theFont->AddVertsForTextInBox2D(textVerts, textBounds, uiDim.y * .026f, text, Rgba8::WHITE, FONT_DEFAULT_ASPECT*.6f, ALIGN_CENTER_LEFT, .03f,FONT_DEFAULT_KERNING);
    }

    if (m_state == GAME_SETTINGS_CALIBRATE) {
        AABB2 drawBounds = uiBound.GetBoxAtTop(.45f);
        drawBounds.ChopBoxOffTop(.4f);
        m_songManager->Render(drawBounds);
		ParticleSystem2D::gParticleSystem2D->Render();
    }

	g_theRenderer->BindDiffuseTexture(g_theFont->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);

	//debug draw
    if (g_isDebugDrawing)
    {
        std::vector<Vertex_PCU> verts;		
		std::string text = Stringf("fps: %.1f\n", 1.f/(float)m_gameClock->GetLastDeltaSeconds());;
		switch (m_state)
		{
		case GAME_ATTRACT:		text+="Attract\n";		break;
		case GAME_MAIN_MENU:	text+= "Main Menu\n";	break;
		case GAME_MUSIC_SELECT:	text+= "Music Select\n";	break;
		case GAME_MUSIC_PLAY:	text+="Music Play\n";		break;
		case GAME_TUTORIAL:		text+= "Controls\n";	break;
		case GAME_SETTINGS:		text+="Settings\n";		break;
		case GAME_CREDITS:		text+="Credits\n";		break;
		}
		if(m_state==GAME_MUSIC_PLAY){
            text += m_songManager->GetDebugTextForCurrentSong();
            text += "\n[Start/Back] to pause\n[A] to start/resume";
		}
		else{ 
			text += "[Back] to back\n"; 
			if((int)m_state<(int)GAME_MUSIC_PLAY){
				text+="[A] to confirm\n";
			}
			if (m_state == GAME_MUSIC_SELECT) {
				text += m_songManager->GetDebugTextForCurrentSong();
			}
			else if (m_state == GAME_SETTINGS) {
				text+="[LB/RB] adjust volume";
			}
		}

        g_theFont->AddVertsForTextInBox2D(verts, uiBound, uiDim.y * .02f, text, Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_TOP_LEFT, .05f, FONT_DEFAULT_KERNING);
        g_theRenderer->BindDiffuseTexture(g_theFont->GetTexture());
        g_theRenderer->DrawVertexArray(verts);
    }

    g_theRenderer->EndCamera(m_uiCamera);
}
