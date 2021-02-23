#pragma once

class RandomNumberGenerator;
class Camera;
class Clock;
class SongManager;
struct AABB2;
struct Vec2;

void GetMenuButtonsInfoFromBounds(AABB2 const& bounds, unsigned int buttonNum,
    AABB2& singleBounds, Vec2& singleTrans);

//////////////////////////////////////////////////////////////////////////
enum GameState
{
	GAME_ATTRACT = 0,
	GAME_MAIN_MENU,
	GAME_MUSIC_SELECT,
	GAME_MUSIC_PLAY,
    GAME_TUTORIAL,
    GAME_SETTINGS,
	GAME_SETTINGS_CALIBRATE,
    GAME_CREDITS,
};

//////////////////////////////////////////////////////////////////////////
class Game 
{
public:
	Game();
	~Game()=default;
	void StartUp();
	void ShutDown();

	void Update();
	void Render()const;

	void StartOfSong();
	void EndOfSong();

	GameState GetCurrentState() const {return m_state;}
	Clock* GetGameClock() const {return m_gameClock;}
	Camera* GetWorldCamera() const {return m_worldCamera;}
	
private:
	bool m_isLoading = true;
	bool m_startedLoading = false;

	Clock* m_gameClock = nullptr;
	GameState m_state = GAME_ATTRACT;
	RandomNumberGenerator* m_RNG = nullptr;
	Camera* m_worldCamera = nullptr;
	Camera* m_uiCamera = nullptr;

	SongManager* m_songManager = nullptr;
	
private:
	void LoadAndInitialize();
	void InitMusicSelectMenu();

	void UpdateForInput();

	void RenderForGame() const;
	void RenderForUI() const;
};