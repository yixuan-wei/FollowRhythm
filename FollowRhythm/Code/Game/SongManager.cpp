#include "Game/SongManager.hpp"
#include "Game/Song.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/CircleButtonList.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Input/XboxController.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Audio/AudioSystem.hpp"

SongManager* SongManager::sSongManager = nullptr;

static Song* sCalibrateSong = nullptr;
static ButtonList sPauseMenu;
static ButtonList sEndMenu;
static const char sScoreDelimiter = '\t';
static const char* sScoreFilePath = "data/log/highscore.txt";

enum sPauseMenuItem
{
    PAUSE_RESUME = 0,
    PAUSE_RESTART,
    PAUSE_QUIT
};

//////////////////////////////////////////////////////////////////////////
static void InitPauseMenuButtons(AABB2 const& bounds)
{
    AABB2 singleBound;
    Vec2 singleTrans;
    GetMenuButtonsInfoFromBounds(bounds, 3, singleBound, singleTrans);

    sPauseMenu.m_buttons.push_back(Button("Resume", true, singleBound));
    singleBound.Translate(singleTrans);
    sPauseMenu.m_buttons.push_back(Button("Restart", false, singleBound));
    singleBound.Translate(singleTrans);
    sPauseMenu.m_buttons.push_back(Button("Quit", false, singleBound));

    sPauseMenu.m_buttonTex = g_theRenderer->CreateOrGetTextureFromFile("data/images/buttons-2d/6-new.png");
}

static void InitEndMenuButtons(AABB2 const& bounds)
{
    sEndMenu.m_buttons.push_back(Button("Back", true, bounds));

    sEndMenu.m_textColor = Rgba8::BLACK;
    sEndMenu.m_buttonTex = g_theRenderer->CreateOrGetTextureFromFile("data/images/buttons-2d/6-new.png");
}

//////////////////////////////////////////////////////////////////////////
static void RenderForPause(AABB2 const& bounds, std::vector<Vertex_PCU>& textVerts)
{
    //overlay background
    g_theRenderer->BindDiffuseTexture((Texture*)nullptr);
    g_theRenderer->DrawAABB2D(bounds, Rgba8(255, 255, 255, 100));

    Vec2 uiDim = bounds.GetDimensions();
    g_theFont->AddVertsForTextInBox2D(textVerts, bounds.GetBoxAtTop(.2f), uiDim.y * .08f, "Pause", Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .05f, FONT_DEFAULT_KERNING);
    sPauseMenu.Render(textVerts);
}

//////////////////////////////////////////////////////////////////////////
SongManager::SongManager(Game* game, char const* musicFolderPath)
    : m_game(game)
{
    if (sSongManager != nullptr) {
        ERROR_AND_DIE("Multiple music manager inited");
    }

    std::vector<std::string> musicList = FilesFindInDirectory(musicFolderPath, "*.mp3");
    for(std::string musicPath : musicList){
        Song* newSong = new Song(musicPath.c_str());
        //TODO for debug music list
        if (!newSong->m_isCalibration) {            
            m_songs.push_back(newSong);
        }
        else {
            sCalibrateSong = newSong;
        }
    }

    sSongManager = this;
    m_timer = new Timer();
    ReadHighScore();

    //init pause menu
    AABB2 bounds = game->GetWorldCamera()->GetBounds();
    AABB2 pBounds = bounds.GetBoxAtBottom(.7f);
    pBounds.ChopBoxOffBottom(3.f/7.f);
    pBounds.ChopBoxOffLeft(.4f);
    pBounds.ChopBoxOffRight(.6667f);
    InitPauseMenuButtons(pBounds);
    //init end menu
    AABB2 endBounds = bounds.GetBoxAtBottom(.15f);
    endBounds.ChopBoxOffBottom(.4f);
    endBounds.ChopBoxOffLeft(.4f);
    endBounds.ChopBoxOffRight(.6667f);
    InitEndMenuButtons(endBounds);
}

//////////////////////////////////////////////////////////////////////////
SongManager::~SongManager()
{
    WriteHighScore();
    for (Song* s : m_songs) {
        delete s;
    }
}

//////////////////////////////////////////////////////////////////////////
void SongManager::InitMusicSelectMenu(CircleButtonList* menu)
{
    AABB2 bounds;
    for (Song* song : m_songs) {
        menu->m_buttons.push_back(Button(song->m_songName, true, bounds));
    }
    menu->UpdateForSelection();
}

//////////////////////////////////////////////////////////////////////////
void SongManager::ClearScoreHistory()
{
    for (Song* s : m_songs) {
        s->m_highestScore = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void SongManager::Update()
{
    UpdateForSong();
    UpdateForInput();
}

//////////////////////////////////////////////////////////////////////////
void SongManager::Render(AABB2 const& bounds) const
{
    std::vector<Vertex_PCU> textVerts;
    m_currentSong->Render(bounds, textVerts);

    if (m_songState == SONG_START) {
        int remains = 1+ (int)m_timer->GetRemainingSeconds();
        AABB2 countdown = bounds;
        countdown.SetDimensions(Vec2(300.f, 300.f));
        g_theFont->AddVertsForTextInBox2D(textVerts, countdown, 200.f, Stringf("%i", remains),
            Rgba8::YELLOW);
    }
    else if (m_songState == SONG_PAUSE) {        
        RenderForPause(bounds, textVerts);
    }
    else if (m_songState == SONG_FINISH) {
        RenderForEnding(bounds, textVerts);
    }

    g_theRenderer->BindDiffuseTexture(g_theFont->GetTexture());
    g_theRenderer->DrawVertexArray(textVerts);
}

//////////////////////////////////////////////////////////////////////////
bool SongManager::StartPlaySong(unsigned int songIndex)
{
    m_currentSongIndex = songIndex;
    m_currentSong = m_songs[m_currentSongIndex];
    if (!m_currentSong->m_isValid) {
        return false;
    }

    m_songState = SONG_START;
    m_timer->SetTimerSeconds(m_game->GetGameClock(), 3.0);
    return true;
}

//////////////////////////////////////////////////////////////////////////
void SongManager::StartCalibration()
{
    m_currentSong = sCalibrateSong;
    m_currentSong->Start(true);
    m_songState = SONG_PLAY;
}

//////////////////////////////////////////////////////////////////////////
void SongManager::StopCalibration()
{
    m_currentSong->Stop();
    m_currentSong = nullptr;
    m_songState = SONG_NULL;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK SongManager::EndOfSong(FMOD_CHANNELCONTROL* channelControl, 
    FMOD_CHANNELCONTROL_TYPE controlType, FMOD_CHANNELCONTROL_CALLBACK_TYPE callbackType, 
    void* commanData1, void* commanData2)
{
    UNUSED(channelControl);
    UNUSED(controlType);
    UNUSED(commanData1);
    UNUSED(commanData2);

    if(callbackType==FMOD_CHANNELCONTROL_CALLBACK_END){
        sSongManager->m_currentSong->AfterPlay();
        sSongManager->m_songState = SONG_FINISH;
    }
    return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
Song* SongManager::GetSongFromSongName(std::string const& songName) const
{
    for (Song* s : m_songs) {
        if (s->m_songName == songName) {
            return s;
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void SongManager::ReadHighScore()
{
    Strings scoreTexts = FileReadLines(sScoreFilePath);
    for (std::string text : scoreTexts) {
        Strings chunks = SplitStringOnDelimiter(text, sScoreDelimiter);
        if (chunks.size() != 2) {
            continue;
        }

        Song* song = GetSongFromSongName(chunks[0]);
        if(song!=nullptr){
            int score = StringConvert(chunks[1].c_str(), -1);
            song->m_highestScore = score;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SongManager::WriteHighScore()
{
    std::string scoreText;
    for (Song* s : m_songs) {
        scoreText += Stringf("%s%c%i\n",s->m_songName.c_str(), sScoreDelimiter,s->m_highestScore);
    }
    if (!FileWriteToDisk(sScoreFilePath, scoreText.data(), scoreText.size())) {
        ERROR_RECOVERABLE("Fail to save high score to disk");
    }
}

//////////////////////////////////////////////////////////////////////////
void SongManager::StartPlayCurrentSong()
{
    m_currentSong = m_songs[m_currentSongIndex];
    m_currentSong->Start();
}

//////////////////////////////////////////////////////////////////////////
void SongManager::UpdateForInput()
{
    XboxController const& controller = g_theInput->GetXboxController(0);
    if (!controller.IsConnected()) {
        return;
    }
    
    if (m_songState == SONG_PLAY) {
        m_currentSong->UpdateForPlayInput();

        if (m_game->GetCurrentState() != GAME_MUSIC_PLAY) {
            return;
        }

        if (controller.GetButtonState(gPauseButton).WasJustPressed() ||
            controller.GetButtonState(gBackButton).WasJustPressed()) {
            g_theAudio->PlaySound(gButtonSFXID, false, gSFXVolume);
            m_currentSong->Pause();
            m_songState = SONG_PAUSE;
        }
    }
    else if (m_songState == SONG_PAUSE && m_game->GetCurrentState()==GAME_MUSIC_PLAY) {
        //confirm selection
        if (controller.GetButtonState(gConfirmButton).WasJustPressed()) {
            sPauseMenuItem curItem = (sPauseMenuItem)sPauseMenu.m_selectedIndex;
            switch(curItem){
            case PAUSE_RESUME:{
                m_currentSong->Resume();
                m_songState = SONG_PLAY;
                break;
            }
            case PAUSE_RESTART:            {
                m_currentSong->Stop();
                StartPlayCurrentSong();
                m_songState = SONG_PLAY;
                break;
            }
            case PAUSE_QUIT:            {
                m_currentSong->Stop();
                m_currentSong = nullptr;
                m_game->EndOfSong();
                m_songState = SONG_NULL;
                break;
            }
            }
            g_theAudio->PlaySound(gButtonSFXID, false, gSFXVolume);
            sPauseMenu.m_selectedIndex = 0;
        }

        //navigation
        sPauseMenu.UpdateForNavigationInput(controller);
    }
    else if (m_songState == SONG_FINISH) {
        if (controller.GetButtonState(gConfirmButton).WasJustPressed()) {
            m_currentSong->UpdateScore();
            m_currentSong = nullptr;
            m_songState = SONG_NULL;
            m_game->EndOfSong();
            g_theAudio->PlaySound(gButtonSFXID, false, gSFXVolume);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SongManager::UpdateForSong()
{
    if (m_songState == SONG_START) {
        if (m_timer->HasElapsed()) {
            m_songState = SONG_PLAY;
            m_currentSong->Start();
        }
    }
    else if (m_songState == SONG_PLAY) {
        m_currentSong->UpdateSoundTime();
        m_currentSong->UpdateForCurrentNotes();
    }
}

//////////////////////////////////////////////////////////////////////////
void SongManager::RenderForEnding(AABB2 const& bounds, std::vector<Vertex_PCU>& textVerts) const
{
    g_theRenderer->BindDiffuseTexture((Texture*)nullptr);
    g_theRenderer->DrawAABB2D(bounds, Rgba8(255,255,255,200));

    Vec2 dim = bounds.GetDimensions();
    
    AABB2 titleBound = bounds.GetBoxAtTop(.2f);
    g_theFont->AddVertsForTextInBox2D(textVerts, titleBound, dim.y*.11f, m_currentSong->m_songName, Rgba8::BLACK, FONT_DEFAULT_ASPECT);

    AABB2 contentBound = bounds.GetBoxAtBottom(.6f);
    g_theFont->AddVertsForTextInBox2D(textVerts, contentBound, dim.y*.05f, m_currentSong->GetEndingTextForSong(), 
        Rgba8::BLACK, FONT_DEFAULT_ASPECT, ALIGN_TOP_CENTER, .05f, FONT_DEFAULT_KERNING);

    if (m_currentSong->m_score > m_currentSong->m_highestScore) {
        AABB2 propBound = bounds.GetBoxAtBottom(.8f);
        propBound.ChopBoxOffBottom(.75f);
        g_theFont->AddVertsForTextInBox2D(textVerts, propBound, dim.y*.08f, "NEW RECORD!!", Rgba8::YELLOW, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .05f, FONT_DEFAULT_KERNING);
    }

    sEndMenu.Render(textVerts);
}

//////////////////////////////////////////////////////////////////////////
std::string SongManager::GetDebugTextForCurrentSong() const
{
    if (m_songState == SONG_NULL) {
        return "Chosen "+m_songs[m_currentSongIndex]->m_songName;
    }
    
    return m_currentSong->GetDebugTextForSong();
}

//////////////////////////////////////////////////////////////////////////
std::string SongManager::GetSelectedSongInfo(unsigned int selectIndex) const
{
    Song* selectSong = m_songs[selectIndex];
    std::string info=Stringf("\
Name:   %s\n\
Author: %s\n\
Album:  %s\n\
Genres: %s\n\
Length: %s\n\
Score:  %i\n\
Difficulty: %s", 
    selectSong->m_songName.c_str(), selectSong->m_author.c_str(), selectSong->m_album.c_str(),
     selectSong->m_genres.c_str(), selectSong->m_length.c_str(), selectSong->m_highestScore,
     selectSong->m_difficulty.c_str());
    return info;
}

//////////////////////////////////////////////////////////////////////////
Texture* SongManager::GetSelectedSongImage(unsigned int selectedIndex) const
{
    Song* selectSong = m_songs[selectedIndex];
    return selectSong->m_bgTexture;
}
