#pragma once

#include <string>
#include <vector>
#include <list>
#include "Engine/Core/EventSystem.hpp"

typedef size_t SoundID;
typedef size_t SoundPlaybackID;
class Clock;
class Note;
class Texture;
class SongManager;
struct AABB2;
struct Vertex_PCU;

std::string GetMusicPathWithoutEXT(std::string const& rawMusicPath);
std::string GetNotesFilePath(std::string const& songFilePath);
std::string GetInfoFilePath(std::string const& songFilePath);
unsigned int GetMilliSecondsFromString(std::string const& timeString);
bool IsNameLeftNode(std::string const& name);
bool IsNameUpNode(std::string const& name); //only for multi-notes
float GetScoreMultiplierFromComboCount(unsigned int comboCount);

//actual worker for a song
class Song
{
    friend class SongManager;

public:
    static float GetAverageCalibrationDeltaTime();

    Song(char const* songName);
    ~Song();

    void UpdateForCurrentNotes();
    void UpdateForPlayInput();
    void Render(AABB2 const& bounds, std::vector<Vertex_PCU>& textVerts) const;

    bool AddScore(EventArgs& args);

    float        GetNoteAgeFromTimeMS(unsigned int startMS) const;    //return [0~1]
    float        GetSongProgress() const;
    unsigned int GetSongElapsedMS() const {return m_elapsedMS;}
    std::string  GetEndingTextForSong() const;
    std::string  GetDebugTextForSong() const;

private:
    void LoadNotesFile();
    void LoadInfoFile();

    void BeforePlay();
    void AfterPlay();
    void UpdateScore();
    void UpdateCombo();

    void Start(bool loop=false);
    void Pause();
    void Resume();
    void Restart(); //Not Used for now
    void Stop();    //Not Used for now

    void UpdateSoundTime();

private:
    std::string m_songPath;
    bool m_isValid = true;
    bool m_isCalibration = false;
    SoundID m_soundID;
    SoundPlaybackID m_soundPlayID;
    bool m_isPlaying = false;
    bool m_isPaused = false;

    std::string m_songName;
    std::string m_author;
    std::string m_album;
    std::string m_link;
    std::string m_genres;
    std::string m_length;    
    std::string m_difficulty;
    Texture* m_bgTexture = nullptr;
    int m_highestScore = 0;

    int m_score = 0;
    unsigned int m_comboCount = 0;
    unsigned int m_maxCombo = 0;
    unsigned int m_perfectCount = 0;
    unsigned int m_goodCount = 0;
    unsigned int m_fairCount = 0;

    unsigned int m_songLength = 0;
    unsigned int m_elapsedMS = 0;

    std::vector<Note*> m_notes;
    std::list<size_t> m_currentNotesIndex;
    size_t m_endNoteIndex = 0;
};