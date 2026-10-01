/*
 * CoA Universal Content Scaling
 * InstanceScaleContext: Implementation of group scaling curves and authoritative encounter locking.
 */

#include "InstanceScaleContext.h"
#include "CoAContentScaling.h"
#include "ContentPackRegistry.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "InstanceProfile.h"
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

    float const actualPlayers = CountEffectivePlayers(map);
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

        it->second.challengeSize = virtualChallenge;
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
    ctx.intendedPlayers = sInstanceProfileRegistry->GetIntendedPlayers(mapId, ctx.difficulty);

    if (map->IsRaid())
    {
        ctx.tier = map->IsHeroic() ? ContentTier::RAID_END : ContentTier::RAID_MID;
    }
    else if (map->IsDungeon())
    {
        ctx.tier = map->IsHeroic() ? ContentTier::DUNGEON_HEROIC : ContentTier::DUNGEON_NORMAL;
    }
    else
    {
        ctx.tier = ContentTier::WORLD;
    }

    uint32 virtualChallenge = 0;
    auto cit = _challengeSizes.find(key);
    if (cit != _challengeSizes.end())
        virtualChallenge = cit->second;

    ctx.challengeSize = virtualChallenge;
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

EncounterScaleSnapshot InstanceScalingMgr::LockEncounterContext(Map* map, EncounterLifecycleSource source, EncounterKey key, EncounterHealthTransferPolicy hpPolicy)
{
    float const actualPlayers = CountEffectivePlayers(map);
    uint32 const mapId = map->GetId();
    uint32 const instanceId = map->GetInstanceId();
    uint64 const mkey = (static_cast<uint64>(mapId) << 32) | instanceId;

    std::lock_guard<std::mutex> lock(_lock);
    InstanceScaleContext& ctx = _contexts[mkey];

    uint32 virtualChallenge = 0;
    auto cit = _challengeSizes.find(mkey);
    if (cit != _challengeSizes.end())
        virtualChallenge = cit->second;

    ctx.effectivePlayers = (virtualChallenge > 0) ? float(virtualChallenge) : actualPlayers;
    if (ctx.intendedPlayers == 0)
        ctx.intendedPlayers = sInstanceProfileRegistry->GetIntendedPlayers(mapId, map->GetDifficulty());

    ctx.CalculateMultipliers(ctx.effectivePlayers / float(ctx.intendedPlayers));

    ctx.lockState = EncounterLockState::ACTIVE;
    ctx.encounterLocked = true;
    ctx.lockEncounterId = key.id;
    ctx.activeEncounterSource = source;
    ctx.activeEncounterKey = key;
    ctx.hpPolicy = hpPolicy;

    EncounterScaleSnapshot snapshot;
    snapshot.effectivePlayers = ctx.effectivePlayers;
    snapshot.intendedPlayers = ctx.intendedPlayers;
    snapshot.healthScale = ctx.healthScale;
    snapshot.damageScale = ctx.damageScale;
    snapshot.healingScale = ctx.healingScale;
    snapshot.absorbScale = ctx.absorbScale;
    snapshot.generation = ++ctx.snapshotGeneration;
    snapshot.valid = true;

    ctx.activeSnapshot = snapshot;

    LOG_INFO("server.loading", "UniversalContentScaling: Encounter {}:{} locked on map {} (instance {}) [gen {}]: {} effective players (intended {}) -> HP x{:.2f}, Dmg x{:.2f}",
             uint32(key.type), key.id, mapId, instanceId, snapshot.generation, snapshot.effectivePlayers, snapshot.intendedPlayers, snapshot.healthScale, snapshot.damageScale);

    return snapshot;
}

void InstanceScalingMgr::ApplySnapshotToBoss(Creature* boss, EncounterScaleSnapshot const& snapshot, EncounterHealthTransferPolicy hpPolicy)
{
    if (!boss || !boss->GetMap())
        return;

    uint32 const mapId = boss->GetMap()->GetId();
    uint32 const instanceId = boss->GetMap()->GetInstanceId();
    uint64 const mkey = (static_cast<uint64>(mapId) << 32) | instanceId;

    {
        std::lock_guard<std::mutex> lock(_lock);
        auto it = _contexts.find(mkey);
        if (it != _contexts.end())
        {
            auto git = it->second.bossAppliedGenerations.find(boss->GetGUID());
            if (git != it->second.bossAppliedGenerations.end() && git->second == snapshot.generation)
            {
                // Already applied authoritative pull stats for this generation
                return;
            }
            it->second.bossAppliedGenerations[boss->GetGUID()] = snapshot.generation;
        }
    }

    sCoAContentScaling->RecalculateEncounterCombatStats(boss, snapshot, hpPolicy);
}

void InstanceScalingMgr::BeginEncounter(Map* map, EncounterLifecycleSource source, EncounterKey key, Creature* boss,
                                       EncounterHealthTransferPolicy hpPolicy)
{
    if (!map || !map->IsDungeon())
        return;

    uint32 const mapId = map->GetId();
    uint32 const instanceId = map->GetInstanceId();
    uint64 const mkey = (static_cast<uint64>(mapId) << 32) | instanceId;

    EncounterScaleSnapshot snapshot;
    bool needApplyBoss = false;

    {
        std::lock_guard<std::mutex> lock(_lock);
        auto it = _contexts.find(mkey);
        if (it != _contexts.end() && it->second.encounterLocked)
        {
            InstanceScaleContext& ctx = it->second;

            // If an authoritative source is already active
            if (ctx.activeEncounterSource <= source)
            {
                if (boss)
                {
                    ctx.bossGuids.insert(boss->GetGUID());
                    snapshot = ctx.activeSnapshot;
                    needApplyBoss = true;
                }
                return;
            }
            else
            {
                // Upgrade from lower priority (e.g. creature fallback) to higher priority (instance script)
                ctx.activeEncounterSource = source;
                ctx.activeEncounterKey = key;
                if (boss)
                {
                    ctx.bossGuids.insert(boss->GetGUID());
                    snapshot = ctx.activeSnapshot;
                    needApplyBoss = true;
                }
            }
        }
    }

    if (needApplyBoss && boss)
    {
        ApplySnapshotToBoss(boss, snapshot, hpPolicy);
        return;
    }

    // New encounter lock
    snapshot = LockEncounterContext(map, source, key, hpPolicy);

    if (boss)
    {
        {
            std::lock_guard<std::mutex> lock(_lock);
            _contexts[mkey].bossGuids.insert(boss->GetGUID());
        }
        ApplySnapshotToBoss(boss, snapshot, hpPolicy);
    }
}

void InstanceScalingMgr::EndEncounter(Map* map, EncounterLifecycleSource source, EncounterKey key, EncounterLockState endState,
                                     ObjectGuid endingGuid)
{
    if (!map || !map->IsDungeon())
        return;

    uint32 const mapId = map->GetId();
    uint32 const instanceId = map->GetInstanceId();
    uint64 const mkey = (static_cast<uint64>(mapId) << 32) | instanceId;

    std::lock_guard<std::mutex> lock(_lock);
    auto it = _contexts.find(mkey);
    if (it == _contexts.end() || !it->second.encounterLocked)
        return;

    InstanceScaleContext& ctx = it->second;

    // Check ownership
    if (ctx.activeEncounterSource == EncounterLifecycleSource::INSTANCE_SCRIPT)
    {
        // Only InstanceScript callbacks (DONE, FAIL, reset) can unlock an InstanceScript encounter!
        if (source != EncounterLifecycleSource::INSTANCE_SCRIPT)
        {
            LOG_DEBUG("server.loading", "UniversalContentScaling: Ignored non-InstanceScript encounter end request on map {} while under InstanceScript lock.", mapId);
            return;
        }

        if (key != ctx.activeEncounterKey)
        {
            LOG_DEBUG("server.loading", "UniversalContentScaling: Encounter key mismatch on end (active {}:{}, incoming {}:{}).",
                      uint32(ctx.activeEncounterKey.type), ctx.activeEncounterKey.id, uint32(key.type), key.id);
            return;
        }

        ctx.lockState = endState;
        ctx.encounterLocked = false;
        ctx.lockEncounterId = 0;
        ctx.bossGuids.clear();
        ctx.bossAppliedGenerations.clear();
        ctx.activeSnapshot.valid = false;
        LOG_INFO("server.loading", "UniversalContentScaling: Encounter {}:{} unlocked via InstanceScript on map {} (instance {})",
                 uint32(key.type), key.id, mapId, instanceId);
        return;
    }

    if (ctx.activeEncounterSource == EncounterLifecycleSource::CREATURE_FALLBACK)
    {
        if (source != EncounterLifecycleSource::CREATURE_FALLBACK)
            return;

        if (!endingGuid.IsEmpty())
        {
            ctx.bossGuids.erase(endingGuid);

            // Council check: are other council bosses still alive and in combat?
            bool anyAliveInCombat = false;
            for (ObjectGuid const& bguid : ctx.bossGuids)
            {
                if (Creature* c = map->GetCreature(bguid))
                {
                    if (c->IsAlive() && c->IsInCombat())
                    {
                        anyAliveInCombat = true;
                        break;
                    }
                }
            }

            if (anyAliveInCombat)
            {
                LOG_DEBUG("server.loading", "UniversalContentScaling: Council member finished combat, but others remain active on map {}.", mapId);
                return;
            }
        }

        ctx.lockState = endState;
        ctx.encounterLocked = false;
        ctx.lockEncounterId = 0;
        ctx.bossGuids.clear();
        ctx.bossAppliedGenerations.clear();
        ctx.activeSnapshot.valid = false;
        LOG_INFO("server.loading", "UniversalContentScaling: Creature fallback encounter {}:{} unlocked on map {} (instance {})",
                 uint32(key.type), key.id, mapId, instanceId);
    }
}

void InstanceScalingMgr::OnEncounterStart(Map* map, Creature* boss, uint32 encounterId)
{
    EncounterLifecycleSource const src = boss ? EncounterLifecycleSource::CREATURE_FALLBACK : EncounterLifecycleSource::INSTANCE_SCRIPT;
    EncounterKeyType const ktype = boss ? EncounterKeyType::CREATURE_ENTRY : EncounterKeyType::INSTANCE_ENCOUNTER;
    BeginEncounter(map, src, { ktype, encounterId }, boss);
}

void InstanceScalingMgr::OnEncounterEnd(Map* map, uint32 encounterId)
{
    EndEncounter(map, EncounterLifecycleSource::INSTANCE_SCRIPT, { EncounterKeyType::INSTANCE_ENCOUNTER, encounterId }, EncounterLockState::COMPLETED);
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
    if (it != _challengeSizes.end())
        return it->second;
    return 0;
}

bool InstanceScalingMgr::HasCalibratedBossHp(uint32 entry, uint8 difficulty) const
{
    if (difficulty >= MAX_RAID_DIFFICULTY)
        return false;

    std::lock_guard<std::mutex> lock(_lock);
    auto it = _bossFlexCache.find(entry);
    if (it == _bossFlexCache.end())
        return false;

    return it->second[difficulty] > 0;
}

uint32 InstanceScalingMgr::GetCalibratedBossHp(uint32 entry, uint8 difficulty, float effectivePlayers) const
{
    if (difficulty >= MAX_RAID_DIFFICULTY)
        return 0;

    std::lock_guard<std::mutex> lock(_lock);
    auto it = _bossFlexCache.find(entry);
    if (it == _bossFlexCache.end())
        return 0;

    uint32 const perPlayerHp = it->second[difficulty];
    if (perPlayerHp == 0)
        return 0;

    return static_cast<uint32>(std::round(float(perPlayerHp) * effectivePlayers));
}

void InstanceScalingMgr::LoadCalibratedBossFlex()
{
    std::lock_guard<std::mutex> lock(_lock);
    _bossFlexCache.clear();

    QueryResult result = WorldDatabase.Query("SELECT entry, difficulty, per_player_health FROM coa_boss_flex");
    if (!result)
        return;

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 const entry = fields[0].Get<uint32>();
        uint8 const diff = fields[1].Get<uint8>();
        uint32 const perPlayerHp = fields[2].Get<uint32>();

        if (diff < MAX_RAID_DIFFICULTY)
        {
            _bossFlexCache[entry][diff] = perPlayerHp;
            ++count;
        }
    } while (result->NextRow());

    LOG_INFO("server.loading", "UniversalContentScaling: Loaded {} calibrated boss flex profiles from coa_boss_flex", count);
}

void InstanceScalingMgr::Clear()
{
    std::lock_guard<std::mutex> lock(_lock);
    _contexts.clear();
    _challengeSizes.clear();
    _bossFlexCache.clear();
}
