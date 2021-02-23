#pragma once

#include <string>
#include <vector>
#include "Engine/Core/Rgba8.hpp"

class SpriteSheet;
class SpriteAnimDefinition;
class Texture;
struct Vec2;

struct Background
{
    std::string mainBG;
    std::string floatyBG;
};

struct FireFlicker
{
public:
    Texture* texture = nullptr;
    Rgba8 color;

    FireFlicker(Texture* tex, Rgba8 const& tint);
};


class AssetManager
{
public:
    static AssetManager* gAssetManager;

    AssetManager(char const* assetFile);

    Background GetRandomBackgroundPaths() const;
    FireFlicker GetRandomFireFlicker() const;
    void GetFireFlickerUVsAtTime(Vec2& uvMins, Vec2& uvMaxs, unsigned int milliSeconds) const;

public:
    std::vector<Background> m_backgrounds;

    std::vector<FireFlicker> m_fireTextures;
    SpriteSheet* m_fireSheet = nullptr;
    SpriteAnimDefinition* m_fireAnim = nullptr;

    SpriteSheet* m_monsterSheet = nullptr;
    SpriteAnimDefinition* m_singleMonsterAnim = nullptr;
    SpriteAnimDefinition* m_singleAttackAnim = nullptr;
    SpriteAnimDefinition* m_singleFinishAnim = nullptr;
    SpriteAnimDefinition* m_multiMonsterAnim = nullptr;
    int m_monsterTailIndex = -1;
};