#include "Game/AssetManager.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/RenderContext.hpp"

static Rgba8 sWarmColor = Rgba8(255, 153, 102);
static Rgba8 sColdColor = Rgba8(51, 204, 255);

AssetManager* AssetManager::gAssetManager = nullptr;

//////////////////////////////////////////////////////////////////////////
static Rgba8 GetColorFromText(std::string const& text)
{
    if (text == "warm") {
        return sWarmColor;
    }
    else if (text == "cold") {
        return sColdColor;
    }
    else {
        return Rgba8::WHITE;
    }
}

//////////////////////////////////////////////////////////////////////////
AssetManager::AssetManager(char const* assetFile)
{
    if (gAssetManager != nullptr) {
        ERROR_RECOVERABLE("Instantiate multiple asset managers");
        return;
    }

    XmlDocument assetDoc;
    XmlError code = assetDoc.LoadFile(assetFile);
    if (code != XmlError::XML_SUCCESS) {
        ERROR_AND_DIE(Stringf("Fail to load asset file %s", assetFile));
    }

    XmlElement* root = assetDoc.RootElement();
    GUARANTEE_OR_DIE(root!=nullptr, Stringf("Root element of %s is null", assetFile));

    //backgrounds
    XmlElement const* backgrounds = root->FirstChildElement("Backgrounds");
    std::string folder = ParseXmlAttribute(*backgrounds, "folder","");
    XmlElement const* bg = backgrounds->FirstChildElement("Background");
    while (bg != nullptr) {
        std::string subfolder = ParseXmlAttribute(*bg, "folder","");
        std::string mainFile = ParseXmlAttribute(*bg, "main", "");
        std::string floaty = ParseXmlAttribute(*bg, "floaty","");
        Background bgStruct;
        bgStruct.floatyBG = folder+subfolder+floaty;
        bgStruct.mainBG = folder+subfolder+mainFile;
        m_backgrounds.push_back(bgStruct);
        g_theRenderer->CreateOrGetTextureFromFile(bgStruct.floatyBG.c_str());
        g_theRenderer->CreateOrGetTextureFromFile(bgStruct.mainBG.c_str());
        bg = bg->NextSiblingElement("Background");
    }

    //fires
    XmlElement const* fires = root->FirstChildElement("Fires");
    std::string fireFolder = ParseXmlAttribute(*fires, "folder", "");
    Texture* defaultTex = nullptr;
    XmlElement const* fire = fires->FirstChildElement("Fire");
    while (fire != nullptr) {
        std::string imgPath = ParseXmlAttribute(*fire, "img","");
        imgPath = fireFolder+imgPath;
        defaultTex = g_theRenderer->CreateOrGetTextureFromFile(imgPath.c_str());
        std::string colorText = ParseXmlAttribute(*fire, "color","");
        m_fireTextures.emplace_back(defaultTex, GetColorFromText(colorText));
        fire = fire->NextSiblingElement("Fire");
    }
    if (defaultTex != nullptr) {
        IntVec2 layout = ParseXmlAttribute(*fires, "layout", IntVec2(1,1));
        m_fireSheet = new SpriteSheet(*defaultTex, layout);
        m_fireAnim = new SpriteAnimDefinition(*m_fireSheet, 0, 59, 3.f);
    }

    //monsters
    XmlElement const* monsters = root->FirstChildElement("Monsters");
    std::string monsterFile = ParseXmlAttribute(*monsters, "file", "");
    IntVec2 layout = ParseXmlAttribute(*monsters, "layout", IntVec2(1,1));
    Texture* monsterTex = g_theRenderer->CreateOrGetTextureFromFile(monsterFile.c_str());
    m_monsterSheet = new SpriteSheet(*monsterTex, layout);
    XmlElement const* mon = monsters->FirstChildElement("Monster");
    while (mon != nullptr) {
        std::string type = ParseXmlAttribute(*mon, "type", "");
        Ints anims;
        anims = ParseXmlAttribute(*mon, "anim", anims);
        if (type == "single") {            
            m_singleMonsterAnim = new SpriteAnimDefinition(*m_monsterSheet, anims, 1.f);
            float scoreDeltaTime = (float)NOTE_SCORE_DELTA_TIME_MS *.001f;
            anims = ParseXmlAttribute(*mon, "attack", anims);
            m_singleAttackAnim = new SpriteAnimDefinition(*m_monsterSheet, anims, scoreDeltaTime, eSpriteAnimPlaybackType::ONCE);
            anims = ParseXmlAttribute(*mon, "finish", anims);
            m_singleFinishAnim = new SpriteAnimDefinition(*m_monsterSheet, anims, scoreDeltaTime, eSpriteAnimPlaybackType::ONCE);
        }
        else if (type == "multi") {
            m_multiMonsterAnim = new SpriteAnimDefinition(*m_monsterSheet, anims, 1.f);
            m_monsterTailIndex = ParseXmlAttribute(*mon, "tail", 0);
        }
        mon = mon->NextSiblingElement("Monster");
    }

    gAssetManager = this;
}

//////////////////////////////////////////////////////////////////////////
Background AssetManager::GetRandomBackgroundPaths() const
{
    int maxIndex = (int)m_backgrounds.size()-1;
    int index = g_theRNG->RollRandomIntInRange(0,maxIndex);
    return m_backgrounds[index];
}

//////////////////////////////////////////////////////////////////////////
FireFlicker AssetManager::GetRandomFireFlicker() const
{
    int maxIndex = (int)m_fireTextures.size()-1;
    int index = g_theRNG->RollRandomIntInRange(0, maxIndex);
    return m_fireTextures[index];
}

//////////////////////////////////////////////////////////////////////////
void AssetManager::GetFireFlickerUVsAtTime(Vec2& uvMins, Vec2& uvMaxs, unsigned int milliSeconds) const
{
    float elapsedSeconds = (float)milliSeconds*.001f;
    m_fireAnim->GetSpriteDefAtTime(elapsedSeconds).GetUVs(uvMins,uvMaxs);
}

//////////////////////////////////////////////////////////////////////////
FireFlicker::FireFlicker(Texture* tex, Rgba8 const& tint)
    : texture(tex)
    , color(tint)
{
}
