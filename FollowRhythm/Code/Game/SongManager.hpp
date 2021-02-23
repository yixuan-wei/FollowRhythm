#pragma once

#include <vector>
#include <string>
#include "ThirdParty/fmod/fmod_common.h"

class Song;
class Game;
class Texture;
class CircleButtonList;
class Timer;
struct AABB2;
struct Vertex_PCU;

enum SongState
{
    SONG_NULL,
    SONG_START,
    SONG_PLAY,
    SONG_PAUSE,
    SONG_FINISH
};

//multiple song and stages control
class SongManager
{
public:
    static SongManager* sSongManager;
    static FMOD_RESULT F_CALLBACK EndOfSong(FMOD_CHANNELCONTROL* channelControl,
        FMOD_CHANNELCONTROL_TYPE controlType,
        FMOD_CHANNELCONTROL_CALLBACK_TYPE callbackType,
        void* commanData1, void* commanData2);

    SongManager(Game* game, char const* musicFolderPath);
    ~SongManager();

    void InitMusicSelectMenu(CircleButtonList* menu);
    void ClearScoreHistory();

    void Update();
    void Render(AABB2 const& bounds) const;
    
    bool StartPlaySong(unsigned int songIndex);
    void StartCalibration();
    void StopCalibration();

    std::string GetDebugTextForCurrentSong() const;
    std::string GetSelectedSongInfo(unsigned int selectIndex) const;
    Texture*    GetSelectedSongImage(unsigned int selectedIndex) const;

private:
    Song* GetSongFromSongName(std::string const& songName) const;

    void ReadHighScore();
    void WriteHighScore();

    void StartPlayCurrentSong();

    void UpdateForInput();
    void UpdateForSong();

    void RenderForEnding(AABB2 const& bounds, std::vector<Vertex_PCU>& textVerts) const;

private:
    Game* m_game = nullptr;
    SongState m_songState = SONG_NULL;
    Timer* m_timer = nullptr;

    std::vector<Song*> m_songs;
    Song* m_currentSong = nullptr;
    unsigned int m_currentSongIndex = 0;
};