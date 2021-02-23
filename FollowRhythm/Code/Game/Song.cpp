#include "Game/Song.hpp"
#include "Game/Note.hpp"
#include "Game/GameCommon.hpp"
#include "Game/SongManager.hpp"
#include "Game/AssetManager.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Input/InputSystem.hpp"

#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/DevConsole.hpp"

static float sTotalCalibDelta = 0.f;
static unsigned int sTotalCalibHit = 0;
static float sLeftStickMoveValue = (NOTE_RENDER_MULTI_DOWN_Y + NOTE_RENDER_MULTI_UP_Y) * .5f;
static float sRightStickMoveValue = (NOTE_RENDER_MULTI_DOWN_Y + NOTE_RENDER_MULTI_UP_Y) * .5f;
static float sInstantRank = 0.f;

static Background sBackground;
static FireFlicker sFireFlicker(nullptr, Rgba8::WHITE);

//////////////////////////////////////////////////////////////////////////
float Song::GetAverageCalibrationDeltaTime()
{
    if (sTotalCalibHit == 0) {
        return 0.f;
    }

    return sTotalCalibDelta / (float)sTotalCalibHit;
}

//////////////////////////////////////////////////////////////////////////
Song::Song(char const* songName)
{
    m_songPath = GetMusicPathWithoutEXT(songName);
    m_soundID = g_theAudio->CreateOrGetSound(songName);
    
    Strings names = SplitStringOnDelimiter(m_songPath, '/');
    m_isCalibration = (names.back() == "Calibration");

    LoadInfoFile();
    LoadNotesFile();

    sBackground = AssetManager::gAssetManager->GetRandomBackgroundPaths();
    sFireFlicker = AssetManager::gAssetManager->GetRandomFireFlicker();
}

//////////////////////////////////////////////////////////////////////////
Song::~Song()
{
    for (Note* n : m_notes) {
        delete n;
    }
    m_notes.clear();
}

//////////////////////////////////////////////////////////////////////////
void Song::UpdateForCurrentNotes()
{
    if (m_isPaused) {
        return;
    }

    //clean out outdated current notes
    for (auto iter = m_currentNotesIndex.begin(); iter!=m_currentNotesIndex.end();) {
        Note* note = m_notes[*iter];
        if (note->IsGarbage()) {
            if (!note->IsScored()) {
                UpdateCombo();
            }
            note->EndToSee();
            m_currentNotesIndex.erase(iter);
            iter = m_currentNotesIndex.begin();
        }
        else {
            iter++;
        }
    }

    //push in new current notes
    m_endNoteIndex = m_currentNotesIndex.empty()?m_endNoteIndex:m_currentNotesIndex.back()+1;
    for (size_t i = m_endNoteIndex; i < m_notes.size(); i++) {
        Note* note = m_notes[i];
        if (note->IsNew()) {
            note->StartToSee();
            m_currentNotesIndex.push_back((unsigned int)i);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void Song::UpdateForPlayInput()
{
    float defaultY = (NOTE_RENDER_MULTI_DOWN_Y + NOTE_RENDER_MULTI_UP_Y) * .5f;
    sLeftStickMoveValue = defaultY;
    sRightStickMoveValue = defaultY;

    if (m_isPaused) {
        return;
    }

    XboxController const& controller = g_theInput->GetXboxController(0);
    if (!controller.IsConnected()) {
        return;
    }

    if (controller.GetButtonState(XBOX_BUTTON_ID_LSHOULDER).WasJustPressed()) { //left single
        g_theEvents->FireEvent("ButtonPressed isLeft=true", EVENT_GAME);
    }
    if (controller.GetButtonState(XBOX_BUTTON_ID_RSHOULDER).WasJustPressed()) { //right single
        g_theEvents->FireEvent("ButtonPressed isLeft=false", EVENT_GAME);
    }    
    
    AnalogJoystick const& lJoystick = controller.GetLeftJoystick();
    float lStickYValue = lJoystick.GetPosition().y;
    std::string text = Stringf("JoystickMoved isLeft=true yValue=%f", lStickYValue);    
    if (lStickYValue > INPUT_JOYSTICK_DEAD_Y) {
        sLeftStickMoveValue = NOTE_RENDER_MULTI_UP_Y;
    }
    else if (lStickYValue < -INPUT_JOYSTICK_DEAD_Y) {
        sLeftStickMoveValue = NOTE_RENDER_MULTI_DOWN_Y;
    }
    g_theEvents->FireEvent(text, EVENT_GAME);

    AnalogJoystick const& rJoystick = controller.GetRightJoystick();
    float rStickYValue = rJoystick.GetPosition().y;
    text = Stringf("JoystickMoved isLeft=false yValue=%f", rStickYValue);    
    if (rStickYValue > INPUT_JOYSTICK_DEAD_Y) {
        sRightStickMoveValue = NOTE_RENDER_MULTI_UP_Y;
    }
    else if (rStickYValue < -INPUT_JOYSTICK_DEAD_Y) {
        sRightStickMoveValue = NOTE_RENDER_MULTI_DOWN_Y;
    }
    g_theEvents->FireEvent(text, EVENT_GAME);
}

//////////////////////////////////////////////////////////////////////////
void Song::Render(AABB2 const& bounds, std::vector<Vertex_PCU>& textVerts) const
{
    if (!m_isCalibration) { //background
        g_theRenderer->BindDiffuseTexture(sBackground.mainBG.c_str());
        g_theRenderer->DrawAABB2D(bounds, Rgba8(150,150,150));
    }
    
    float bgFlickerFactor = sInstantRank * .01f;
    unsigned char alphaFlicker = (unsigned char)Interpolate(150.f, 240.f, bgFlickerFactor);
    Rgba8 flickerColor = sFireFlicker.color;
    flickerColor.a = alphaFlicker;
    //center
    float halfWidth = RENDER_CENTER_FRACTION * (bounds.maxs.x-bounds.mins.x) *.5f;
    AABB2 centerBound(-halfWidth, bounds.mins.y, halfWidth, bounds.maxs.y);
    g_theRenderer->BindDiffuseTexture((Texture*)nullptr);
    g_theRenderer->DrawAABB2D(centerBound, flickerColor);

    if (!m_isCalibration) { 
         //base
        float baseWidth = 1.6f * halfWidth;
        Vec2 baseDim(baseWidth, baseWidth);
        g_theRenderer->BindDiffuseTexture("data/images/base.png");
        AABB2 baseBound = centerBound.GetBoxAtBottom(0.f, baseWidth);
        baseBound.SetDimensions(baseDim);
        Rgba8 baseColor = Lerp(flickerColor, Rgba8(255,255,255,alphaFlicker), bgFlickerFactor);
        g_theRenderer->DrawAABB2D(baseBound, baseColor);

        //joystick block        
        //left
        AABB2 leftBlock(-halfWidth, -NOTE_RENDER_HALF_SIZE, -halfWidth + 30.f, NOTE_RENDER_HALF_SIZE);
        leftBlock.Translate(Vec2(0.f, sLeftStickMoveValue));
        g_theRenderer->DrawAABB2D(leftBlock, Rgba8::GREEN, Vec2(0.f, 1.f), Vec2(1.f, 0.f));
        //right
        AABB2 rightBlock(halfWidth - 30.f, -NOTE_RENDER_HALF_SIZE, halfWidth, NOTE_RENDER_HALF_SIZE);
        rightBlock.Translate(Vec2(0.f, sRightStickMoveValue));
        g_theRenderer->DrawAABB2D(rightBlock, Rgba8::GREEN, Vec2(0.f, 1.f), Vec2(1.f, 0.f));

        //fire
        Vec2 uvMins, uvMaxs;
        AssetManager::gAssetManager->GetFireFlickerUVsAtTime(uvMins, uvMaxs, m_elapsedMS);        
        AABB2 fireBounds = centerBound.GetBoxAtBottom(0.f, baseWidth * 2.f);
        fireBounds.ChopBoxOffBottom(.5f);
        float dilationRate = Interpolate(1.f, 2.2f, bgFlickerFactor);
        fireBounds.SetDimensions(baseDim * dilationRate);
        fireBounds.Translate(Vec2(-5.f,0.f));
        g_theRenderer->BindDiffuseTexture(sFireFlicker.texture);
        g_theRenderer->DrawAABB2D(fireBounds, flickerColor, uvMins, uvMaxs);

        //HUD
        //progress bar
        g_theRenderer->BindDiffuseTexture((Texture*)nullptr);
        AABB2 progressBar = bounds.GetBoxAtTop(.05f);
        g_theRenderer->DrawAABB2D(progressBar, Rgba8(100, 100, 100, 255));
        float progress = GetSongProgress();
        progressBar.ChopBoxOffRight(1.f - GetSongProgress());
        g_theRenderer->BindDiffuseTexture("data/images/buttons-2d/progress.png");
        g_theRenderer->DrawAABB2D(progressBar, Rgba8::WHITE, Vec2::ZERO, Vec2(progress, 1.f));

        //combo
        AABB2 ComboBound = bounds.GetBoxAtRight(.6f);
        ComboBound.ChopBoxOffRight(.6667f);
        ComboBound.ChopBoxOffTop(.1f);
        ComboBound.ChopBoxOffBottom(.8f);
        dilationRate = Interpolate(1.f,1.6f, bgFlickerFactor);
        float textHeight = ComboBound.GetDimensions().y * .2f;
        AABB2 scoreBound = ComboBound.ChopBoxOffTop(.25f);
        AABB2 comboCountBound = ComboBound.ChopBoxOffBottom(.5f);
        Rgba8 comboColor = Lerp(Rgba8(255,223,0),Rgba8::WHITE, GetScoreMultiplierFromComboCount(m_comboCount)*.2f );
        g_theFont->AddVertsForTextInBox2D(textVerts, scoreBound, textHeight*dilationRate,
            Stringf("%i", m_score), Rgba8::WHITE, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .1f, FONT_DEFAULT_KERNING);
        g_theFont->AddVertsForTextInBox2D(textVerts, ComboBound, textHeight, "Combo",
            comboColor, FONT_DEFAULT_ASPECT, ALIGN_BOTTOM_CENTER, .1f, FONT_DEFAULT_KERNING);
        g_theFont->AddVertsForTextInBox2D(textVerts, comboCountBound, textHeight*dilationRate,
            Stringf("%u", m_comboCount), comboColor, FONT_DEFAULT_ASPECT, ALIGN_CENTERED, .1f, FONT_DEFAULT_KERNING);
    }    

    //draw notes
    g_theRenderer->BindDiffuseTexture(&AssetManager::gAssetManager->m_monsterSheet->GetTexture());
    for (auto iter = m_currentNotesIndex.begin(); iter != m_currentNotesIndex.end(); iter++) {
        m_notes[*iter]->Render(bounds);
    }    
}

//////////////////////////////////////////////////////////////////////////
bool Song::AddScore(EventArgs& args)
{
    float rank = args.GetValue("rank", 0.f);
    sInstantRank = rank;
    std::string debugText = Stringf("Rank: %.0f\n", rank); 
    
    if(rank>=COMBO_GOOD_RANK){
        if (rank >= COMBO_PERFECT_RANK) {
            m_perfectCount++;
            debugText +="Perfect";
        }
        else {
            m_goodCount++;
            debugText += "Good";
        }
        m_comboCount++;
        float multiplier = GetScoreMultiplierFromComboCount(m_comboCount);
        rank*= multiplier;
    }
    else{
        UpdateCombo();
        if (rank >= COMBO_FAIR_RANK) {
            debugText += "OK";            
            m_fairCount++;
        }
        else {
            debugText += "MISS";
        }
    }

    float multiFactor = args.GetValue("multi", 1.f);
    rank *= multiFactor;
    m_score += (int)rank;

    DebugAddScreenText(Vec4(.5f, .5f, 0.f, 0.f), Vec2(.5f, .5f), 30.f, Rgba8::RED, Rgba8::RED, .5f, debugText.c_str());

    //calibration
    sTotalCalibHit++;
    sTotalCalibDelta += args.GetValue("delta", 0.f);

    return true;
}

//////////////////////////////////////////////////////////////////////////
float Song::GetNoteAgeFromTimeMS(unsigned int startMS) const
{
    return ((float)(m_elapsedMS + NOTE_RENDER_MAX_TIME_MS)-(float)startMS)/(float)NOTE_RENDER_MAX_TIME_MS;
}

//////////////////////////////////////////////////////////////////////////
void Song::LoadNotesFile()
{
    std::string notesFile = GetNotesFilePath(m_songPath);
    Strings lines = FileReadLines(notesFile);
    if (lines.empty()) {
        m_isValid = false;
        return;
    }

    for (size_t i = 1; i < lines.size(); i++) {
        std::string line = lines[i];

        Strings trunks = SplitStringOnDelimiter(line,'\t');
        if (trunks.size() == 6) {        
            unsigned int start = GetMilliSecondsFromString(trunks[1]);
            unsigned int duration = GetMilliSecondsFromString(trunks[2]);
            m_notes.push_back(Note::CreateNote(this,trunks[0], start, duration));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void Song::LoadInfoFile()
{
    std::string infoFile = GetInfoFilePath(m_songPath);
    XmlDocument infoDoc;
    XmlError code = infoDoc.LoadFile(infoFile.c_str());
    if(code != XmlError::XML_SUCCESS){
        g_theConsole->PrintString(Rgba8::RED, Stringf("loading %s failed", infoFile.c_str()));
        m_isValid = false;
        return;
    }

    NamedStrings infoStrings;
    infoStrings.PopulateFromXmlElementAttributes(*infoDoc.RootElement());
    m_songName = infoStrings.GetValue("name","Null");
    m_author = infoStrings.GetValue("author","Null");
    m_album = infoStrings.GetValue("album", "Null");
    m_link = infoStrings.GetValue("link","Null");
    m_genres = infoStrings.GetValue("genres", "");
    m_length = infoStrings.GetValue("length","00:00");
    m_difficulty = infoStrings.GetValue("difficulty","-");
    m_songLength = GetMilliSecondsFromString(m_length);
    std::string bgTexPath = infoStrings.GetValue("background","White");
    m_bgTexture = g_theRenderer->CreateOrGetTextureFromFile(bgTexPath.c_str());
}

//////////////////////////////////////////////////////////////////////////
void Song::BeforePlay()
{
    g_theEvents->RegisterMethodEvent("AddScore", this, &Song::AddScore, "add note score to song", EVENT_GAME);
    sBackground = AssetManager::gAssetManager->GetRandomBackgroundPaths();
    sFireFlicker = AssetManager::gAssetManager->GetRandomFireFlicker();
    m_elapsedMS = 0;
    m_score = 0;
    m_comboCount = 0;
    m_maxCombo = 0;
    m_perfectCount = 0;
    m_goodCount = 0;
    m_fairCount= 0;
    m_endNoteIndex = 0;
    m_isPlaying = true;
    m_isPaused = false;

    sTotalCalibHit=0;
    sTotalCalibDelta = 0.f;
}

//////////////////////////////////////////////////////////////////////////
void Song::AfterPlay()
{
    g_theEvents->UnsubscribeObject(this);
 
    UpdateCombo();
    m_endNoteIndex = 0;
    m_currentNotesIndex.clear();
    m_isPlaying = false;
    m_elapsedMS = 0;
}

//////////////////////////////////////////////////////////////////////////
void Song::UpdateScore()
{    
    m_highestScore = m_highestScore > m_score ? m_highestScore : m_score;
}

//////////////////////////////////////////////////////////////////////////
void Song::UpdateCombo()
{
    m_maxCombo = m_maxCombo > m_comboCount ? m_maxCombo : m_comboCount;
    m_comboCount = 0;
}

//////////////////////////////////////////////////////////////////////////
void Song::Start(bool loop)
{
    BeforePlay();
    m_soundPlayID = g_theAudio->PlaySound(m_soundID, loop, gMusicVolume);
    g_theAudio->SetSoundCallback(m_soundPlayID, SongManager::EndOfSong);    
}

//////////////////////////////////////////////////////////////////////////
void Song::Pause()
{
    //m_pausedPos = g_theAudio->GetSoundPosition(m_soundPlayID);
    g_theAudio->SetSoundPaused(m_soundPlayID,true);
    m_isPaused = true;
}

//////////////////////////////////////////////////////////////////////////
void Song::Resume()
{    
    m_isPaused = false;
    g_theAudio->SetSoundPaused(m_soundPlayID,false);
}

//////////////////////////////////////////////////////////////////////////
void Song::Restart()
{
    g_theAudio->SetSoundPosition(m_soundPlayID,0);
    m_elapsedMS = 0;
    Resume();
}

//////////////////////////////////////////////////////////////////////////
void Song::Stop()
{
    g_theAudio->StopSound(m_soundPlayID);
    AfterPlay();
}

//////////////////////////////////////////////////////////////////////////
void Song::UpdateSoundTime()
{
    if (m_isPlaying && !m_isPaused) {
         unsigned int newMS = g_theAudio->GetSoundPosition(m_soundPlayID);
         if (m_elapsedMS > newMS) {
             m_endNoteIndex = 0;
         }
         m_elapsedMS = newMS;
         if (sInstantRank > 1.f) {
             sInstantRank-=1.f;
         }
    }
}

//////////////////////////////////////////////////////////////////////////
float Song::GetSongProgress() const
{
    return (float)m_elapsedMS/(float)m_songLength;
}

//////////////////////////////////////////////////////////////////////////
std::string Song::GetEndingTextForSong() const
{
    std::string text = Stringf("Score: %i\nMaxCombo: %u\n\nPerfect: %u\nGood: %u\nFair: %u\nMiss: %u", 
        m_score, m_maxCombo, m_perfectCount, m_goodCount, m_fairCount, 
        (unsigned int)m_notes.size()-m_perfectCount-m_goodCount-m_fairCount);
    return text;
}

//////////////////////////////////////////////////////////////////////////
std::string Song::GetDebugTextForSong() const
{
    std::string text = Stringf("%s\n%s\n%.3f: %u/%u\n%u\nIsPlaying: %s\nScore: %i", 
        m_songName.c_str(), m_author.c_str(), GetSongProgress(),
        m_elapsedMS, m_songLength,
        g_theAudio->GetSoundPosition(m_soundPlayID),
        m_isPlaying ? "true" : "false", m_score);
    return text;
}

//////////////////////////////////////////////////////////////////////////
std::string GetMusicPathWithoutEXT(std::string const& rawMusicPath)
{
    Strings paths = SplitStringOnDelimiter(rawMusicPath,'.');
    return paths[0];
}

//////////////////////////////////////////////////////////////////////////
std::string GetNotesFilePath(std::string const& songFilePath)
{
    Strings paths = SplitStringOnDelimiter(songFilePath, '/');
    std::string name = paths.back();
    paths.pop_back();
    return CombineStringsWithDelimiter(paths,'/') + "/notes/"+name+".csv";
}

//////////////////////////////////////////////////////////////////////////
std::string GetInfoFilePath(std::string const& songFilePath)
{
    Strings paths = SplitStringOnDelimiter(songFilePath, '/');
    std::string name = paths.back();
    paths.pop_back();
    return CombineStringsWithDelimiter(paths, '/') + "/info/" + name + ".xml";
}

//////////////////////////////////////////////////////////////////////////
unsigned int GetMilliSecondsFromString(std::string const& timeString)
{
    Strings minutes = SplitStringOnDelimiter(timeString, ':');
    unsigned int m = StringConvert(minutes[0].c_str(), 0);
    Strings seconds = SplitStringOnDelimiter(minutes[1], '.');
    unsigned int s = StringConvert(seconds[0].c_str(), 0);
    unsigned int ms = StringConvert(seconds[1].c_str(), 0);
    return ((unsigned int)60*m + s)*(unsigned int)1000 + ms;
}

//////////////////////////////////////////////////////////////////////////
bool IsNameLeftNode(std::string const& name)
{
    if (name[0] == 'L' || name[0] == 'l') {
        return true;
    }
    else return false;
}

//////////////////////////////////////////////////////////////////////////
bool IsNameUpNode(std::string const& name)
{
    if (name.size() < 2) {
        return false;
    }
    if (name[1] == 'U' || name[1] == 'u') {
        return true;
    }
    else return false;
}

//////////////////////////////////////////////////////////////////////////
float GetScoreMultiplierFromComboCount(unsigned int comboCount)
{
    return Clamp((float)comboCount/(float)COMBO_SINGLE_RATE_COUNT, 1.f, COMBO_MAX_RATE);
}
