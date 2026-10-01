/*
 * CoA Universal Content Scaling
 * InstanceScaleContext: Group and instance scaling context with encounter snapshot locking.
 */

#ifndef COA_INSTANCE_SCALE_CONTEXT_H
#define COA_INSTANCE_SCALE_CONTEXT_H

#include "ContentEra.h"
#include "ContentTier.h"
#include "DBCEnums.h"
#include "Define.h"
#include <cmath>
#include <mutex>
#include <unordered_map>

class Map;
class Player;
class Unit;
class Creature;

struct InstanceScaleContext
{
    uint32 mapId{0};
    uint32 instanceId{0};

    ContentEra era{ContentEra::Classic};
    ContentTier tier{ContentTier::DUNGEON_NORMAL};

    uint32 intendedPlayers{5};
    float effectivePlayers{5.0f};

    Difficulty difficulty{DUNGEON_DIFFICULTY_NORMAL};

    float healthScale{1.0f};
    float damageScale{1.0f};
    float healingScale{1.0f};
    float absorbScale{1.0f};

    bool encounterLocked{false};
    uint32 lockEncounterId{0};

    void CalculateMultipliers(float realRatio);
};

class InstanceScalingMgr
{
public:
    static InstanceScalingMgr* Instance();

    // Context management per instance map
    InstanceScaleContext GetOrCreateContext(Map* map);
    InstanceScaleContext GetContext(uint32 mapId, uint32 instanceId);

    // Count participants in map
    float CountEffectivePlayers(Map* map) const;

    // Encounter lifecycle
    void OnEncounterStart(Map* map, Creature* boss, uint32 encounterId);
    void OnEncounterEnd(Map* map, uint32 encounterId);

    // Challenge mode setting (0 = Adaptive, 1 = Solo, 2 = 2-player, 5 = 5-player, etc.)
    void SetChallengeSize(uint32 mapId, uint32 instanceId, uint32 virtualSize);
    uint32 GetChallengeSize(uint32 mapId, uint32 instanceId) const;

    // Boss flex health integration (priority: explicit encounter override > coa_boss_flex > generic instance scaling)
    uint32 GetCalibratedBossHp(uint32 entry, uint8 difficulty, float effectivePlayers) const;
    bool HasCalibratedBossHp(uint32 entry, uint8 difficulty) const;
    void LoadCalibratedBossFlex();

    void Clear();

private:
    InstanceScalingMgr() = default;

    mutable std::mutex _lock;
    // key: (mapId << 32) | instanceId
    std::unordered_map<uint64, InstanceScaleContext> _contexts;
    std::unordered_map<uint64, uint32> _challengeSizes;

    // coa_boss_flex cache: entry -> perPlayer[4]
    std::unordered_map<uint32, std::array<uint32, MAX_RAID_DIFFICULTY>> _bossFlexCache;
};

#define sInstanceScalingMgr InstanceScalingMgr::Instance()

#endif // COA_INSTANCE_SCALE_CONTEXT_H
