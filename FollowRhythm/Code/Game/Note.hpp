#pragma once

#include <string>

struct AABB2;
class Song;

enum NoteType
{
    NOTE_SINGLE,
    NOTE_MULTIPLE
};

class Note
{
public:
    static Note* CreateNote(Song* song, std::string const& noteName, unsigned int startMS, unsigned int duration);

    Note(Song* song, NoteType type, bool isLeft, unsigned int startSeconds);
    virtual ~Note() = default;

    virtual void StartToSee() = 0;
    virtual void EndToSee() = 0;
    virtual void Render(AABB2 const& bounds) const = 0;

    virtual unsigned int GetRenderBeginMS() const = 0; 
    virtual unsigned int GetRenderEndMS() const = 0;
    virtual bool IsScored() const = 0;
    
    bool IsGarbage() const;
    bool IsNew() const;

protected:
    Song* m_song = nullptr;
    NoteType m_type = NOTE_SINGLE;
    bool m_isLeft = true;
    unsigned int m_startMS = 0;
};