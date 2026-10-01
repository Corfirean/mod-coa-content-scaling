/*
 * CoA Universal Content Scaling
 * InstanceScaleContext: Implementation of group scaling curves and encounter locking.
 */

#include "InstanceScaleContext.h"
#include "ContentPackRegistry.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "QueryResult.h"
#include <algorithm>

void InstanceScaleContext::CalculateMultipliers(float realRatio)
{
    float const ratio = std::clamp<float>(realRatio, 0.025f, 1.0f);

    // Health curve: near-linear, safe minimum 5% to prevent trivialized HP
    healthScale = std::max<float>(0.05f, std::pow(ratio, 0.95f));

    // Damage curve: sub-linear with 25% floor so encounter mechanics maintain bite
    damageScale = std::clamp<float>(0.25f + 0.75f * std::pow(ratio, 0.65f), 0.25f, 1.0f);

    // Healing curve: 20% floor
    healingScale = std::max<float>(0.20f, std::pow(ratio, 0.90f));

    // Absorb curve
    absorbScale = damageScale;
}

InstanceScalingMgr* InstanceScalingMgr::Instance()
{
    static InstanceScalingMgr instance;
    return &instance;
}

float InstanceScalingMgr::CountEffectivePlayers(Map* map) const
{
    if (!map)
        return 1.0f;

    float count = 0.0f;
    map->DoForAllPlayers([&count](Player* player)
    {
        if (!player || player->IsGameMaster())
            return;

        // Both human players and playerbots count with full 1.0 weight
        count += 1.0f;
    });

    return std::max(1.0f, count);
}

InstanceScaleContext InstanceScalingMgr::GetOrCreateContext(Map* map)
{
    if (!map)
        return InstanceScaleContext{};

    uint32 const mapId = map->GetId();
    uint32 const instanceId = map->GetInstanceId();
    uint64 const key = (static_cast<uint64>(mapId) << 32) | instanceId;

    std::lock_guard<std::mutex> lock(_lock);
    auto it = _contexts.find(key);
    if (it != _contexts.end())
    {
        // If combat is already locked, return frozen snapshot
        if (it->second.encounterLocked)
            return it->second;

        // If not in combat, update current participant count for display
        uint32 virtualChallenge = 0;
        auto cit = _challengeSizes.find(key);
        if (cit != _challengeSizes.end())
            virtualChallenge = cit->second;

        float const actualPlayers = CountEffectivePlayers(map);
        it->second.effectivePlayers = (virtualChallenge > 0) ? float(virtualChallenge) : actualPlayers;
        it->second.CalculateMultipliers(it->second.effectivePlayers / float(it->second.intendedPlayers));
        return it->second;
    }

    // New context
    InstanceScaleContext ctx;
    ctx.mapId = mapId;
    ctx.instanceId = instanceId;
    ctx.era = sContentPackRegistry->ResolveEraForMap(mapId);
    ctx.difficulty = map->GetDifficulty();

    if (map->IsRaid())
    {
        ctx.intendedPlayers = (ctx.difficulty == RAID_DIFFICULTY_10MAN_NORMAL ||
                               ctx.difficulty == RAID_DIFFICULTY_10MAN_HEROIC) ? 10 : 25;
        // Classic raids default to 40 unless 10/20 man
        if (ctx.era == ContentEra::Classic)
        {
            if (mapId == 409 || mapId == 469 || mapId == 509 || mapId == 531) // MC, BWL, AQ40, Naxx
                ctx.intendedPlayers = 40;
            else if (mapId == 309 || mapId == 509) // ZG, AQ20
                ctx.intendedPlayers = 20;
            else if (mapId == 249) // Onyxia Classic
                ctx.intendedPlayers = 40;
        }
        ctx.tier = map->IsHeroic() ? ContentTier::RAID_END : ContentTier::RAID_MID;
    }
    else
    {
        ctx.intendedPlayers = 5;
        ctx.tier = map->IsHeroic() ? ContentTier::DUNGEON_HEROIC : ContentTier::DUNGEON_NORMAL;
    }

    uint32 virtualChallenge = 0;
    auto cit = _challengeSizes.find(key);
    if (cit != _challengeSizes.end())
        virtualChallenge = cit->second;

    float const actualPlayers = CountEffectivePlayers(map);
    ctx.effectivePlayers = (virtualChallenge > 0) ? float(virtualChallenge) : actualPlayers;
    ctx.CalculateMultipliers(ctx.effectivePlayers / float(ctx.intendedPlayers));

    _contexts[key] = ctx;
    return ctx;
}

InstanceScaleContext InstanceScalingMgr::GetContext(uint32 mapId, uint32 instanceId)
{
    uint64 const key = (static_cast<uint64>(mapId) << 32) | instanceId;
    std::lock_guard<std::mutex> lock(_lock);
    auto it = _contexts.find(key);
    if (it != _contexts.end())
        return it->second;
    return InstanceScaleContext{};
}

void InstanceScalingMgr::OnEncounterStart(Map* map, Creature* /*boss*/, uint32 encounterId)
{
    if (!map)
        return;

    uint32 const mapId = map->GetId();
    uint32 const instanceId = map->GetInstanceId();
    uint64 const key = (static_cast<uint64>(mapId) << 32) | instanceId;

    std::lock_guard<std::mutex> lock(_lock);
    InstanceScaleContext& ctx = _contexts[key];
    if (ctx.encounterLocked)
        return; // Already locked by another boss / pull

    ctx.encounterLocked = true;
    ctx.lockEncounterId = encounterId;

    uint32 virtualChallenge = 0;
    auto cit = _challengeSizes.find(key);
    if (cit != _challengeSizes.end())
        virtualChallenge = cit->second;

    float const actualPlayers = CountEffectivePlayers(map);
    ctx.effectivePlayers = (virtualChallenge > 0) ? float(virtualChallenge) : actualPlayers;
    ctx.CalculateMultipliers(ctx.effectivePlayers / float(ctx.intendedPlayers));

    LOG_INFO("server.loading", "UniversalContentScaling: Encounter {} locked on map {} with {} effective players (intended {}) -> HP x{:.2f}, Dmg x{:.2f}",
             encounterId, mapId, ctx.effectivePlayers, ctx.intendedPlayers, ctx.healthScale, ctx.damageScale);
}

void InstanceScalingMgr::OnEncounterEnd(Map* map, uint32 encounterId)
{
    if (!map)
        return;

    uint32 const mapId = map->GetId();
    uint32 const instanceId = map->GetInstanceId();
    uint64 const key = (static_cast<uint64>(mapId) << 32) | instanceId;

    std::lock_guard<std::mutex> lock(_lock);
    auto it = _contexts.find(key);
    if (it != _contexts.end() && it->second.encounterLocked)
    {
        it->second.encounterLocked = false;
        it->second.lockEncounterId = 0;
        LOG_INFO("server.loading", "UniversalContentScaling: Encounter {} unlocked on map {}", encounterId, mapId);
    }
}

void InstanceScalingMgr::SetChallengeSize(uint32 mapId, uint32 instanceId, uint32 virtualSize)
{
    uint64 const key = (static_cast<uint64>(mapId) << 32) | instanceId;
    std::lock_guard<std::mutex> lock(_lock);
    if (virtualSize == 0)
        _challengeSizes.erase(key);
    else
        _challengeSizes[key] = virtualSize;
}

uint32 InstanceScalingMgr::GetChallengeSize(uint32 mapId, uint32 instanceId) const
{
    uint64 const key = (static_cast<uint64>(mapId) << 32) | instanceId;
    std::lock_guard<std::mutex> lock(_lock);
    auto it = _challengeSizes.find(key);
    return (it != _challengeSizes.end()) ? it->second : 0;
}

void InstanceScalingMgr::LoadCalibratedBossFlex()
{
    std::lock_guard<std::mutex> lock(_lock);
    _bossFlexCache.clear();

    if (QueryResult result = WorldDatabase.Query("SELECT entry, hp_d0, hp_d1, hp_d2, hp_d3 FROM coa_boss_flex"))
    {
        do
        {
            Field* f = result->Fetch();
            uint32 const entry = f[0].Get<uint32>();
            std::array<uint32, MAX_RAID_DIFFICULTY>& row = _bossFlexCache[entry % 100000];
            for (uint8 i = 0; i < MAX_RAID_DIFFICULTY; ++i)
                row[i] = f[1 + i].Get<uint32>();
        } while (result->NextRow());
    }

    LOG_INFO("server.loading", ">> UniversalContentScaling: Loaded {} calibrated boss flex profiles",
             uint32(_bossFlexCache.size()));
}

bool InstanceScalingMgr::HasCalibratedBossHp(uint32 entry, uint8 difficulty) const
{
    if (difficulty >= MAX_RAID_DIFFICULTY)
        return false;

    std::lock_guard<std::mutex> lock(_lock);
    auto it = _bossFlexCache.find(entry % 100000);
    return (it != _bossFlexCache.end() && it->second[difficulty] > 0);
}

uint32 InstanceScalingMgr::GetCalibratedBossHp(uint32 entry, uint8 difficulty, float effectivePlayers) const
{
    if (difficulty >= MAX_RAID_DIFFICULTY)
        return 0;

    std::lock_guard<std::mutex> lock(_lock);
    auto it = _bossFlexCache.find(entry % 100000);
    if (it == _bossFlexCache.end())
        return 0;

    uint32 const perPlayer = it->second[difficulty];
    if (perPlayer == 0)
        return 0;

    uint64 const total = static_cast<uint64>(perPlayer) * static_cast<uint64>(std::max(1.0f, effectivePlayers));
    return static_cast<uint32>(std::min<uint64>(total, std::numeric_limits<uint32>::max()));
}

void InstanceScalingMgr::Clear()
{
    std::lock_guard<std::mutex> lock(_lock);
    _contexts.clear();
    _challengeSizes.clear();
    _bossFlexCache.clear();
}
