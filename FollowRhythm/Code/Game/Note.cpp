#include "Game/Note.hpp"
#include "Game/MultiNotes.hpp"
#include "Game/SingleNote.hpp"
#include "Game/Song.hpp"

//////////////////////////////////////////////////////////////////////////
Note* Note::CreateNote(Song* song, std::string const& noteName, unsigned int startMS, unsigned int duration)
{
    bool isLeft = IsNameLeftNode(noteName);
    if (duration != 0.0) {
        bool isUp = IsNameUpNode(noteName);
        return (Note*)new MultiNotes(song, isLeft, isUp, startMS, duration);
    }
    else {
        return (Note*)new SingleNote(song, isLeft, startMS);
    }
}

//////////////////////////////////////////////////////////////////////////
Note::Note(Song* song, NoteType type, bool isLeft, unsigned int startMS)
    : m_type(type)
    , m_isLeft(isLeft)
    , m_startMS(startMS)
    , m_song(song)
{

}

//////////////////////////////////////////////////////////////////////////
bool Note::IsGarbage() const
{
    return (GetRenderEndMS() <= m_song->GetSongElapsedMS());
}

//////////////////////////////////////////////////////////////////////////
bool Note::IsNew() const
{
    unsigned int elapsedMS = m_song->GetSongElapsedMS();
    return (GetRenderBeginMS() <= elapsedMS) && (GetRenderEndMS() > elapsedMS);
}
