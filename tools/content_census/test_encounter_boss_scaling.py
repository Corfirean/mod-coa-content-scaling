import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def extract_function(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == '{':
            depth += 1
        elif source[index] == '}':
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise RuntimeError(f'Unclosed function: {signature}')


def main():
    compiler = shutil.which(os.environ.get('CXX', 'cl.exe' if os.name == 'nt' else 'c++'))
    assert compiler, 'Enable a C++20 compiler'

    master = (ROOT / 'src/CoAContentScaling.cpp').read_text(encoding='utf-8')
    extracted_methods = '\n\n'.join(
        extract_function(master, sig)
        for sig in (
            'CoAContentScaling* CoAContentScaling::Instance()',
            'void RescaleLootDamageRequirement(',
            'void CoAContentScaling::ApplyCombatBudget(',
            'void CoAContentScaling::ApplyCreatureScaling(',
            'void CoAContentScaling::RecalculateEncounterCombatStats(',
            'void CoAContentScaling::RefreshDungeonCreatures(',
            'void CoAContentScaling::OnResolveLfgQueuePolicy(',
            'void CoAContentScaling::OnLfgProposalMadeGroup(',
            'std::optional<PendingInstanceScalePolicy> CoAContentScaling::ConsumePendingInstancePolicy(',
            'void CoAContentScaling::PurgeExpiredPendingPolicies()',
            'void CoAContentScaling::OnInstanceMapCreated(',
            'void CoAContentScaling::SetPlayerLfgMode(',
            'lfg::LfgCompositionMode CoAContentScaling::GetPlayerLfgMode(',
            'bool CoAContentScaling::SetPlayerLfgChallenge(',
            'uint32 CoAContentScaling::GetPlayerLfgChallenge(',
        )
    )

    with tempfile.TemporaryDirectory(prefix='coa-encounter-boss-scaling-') as folder:
        out = Path(folder)
        for name in (
            'CoAContentScaling',
            'CombatBudgetProfile',
            'CreatureScaleContext',
            'InstanceScaleContext',
            'ContentPackRegistry',
            'ProgressionLayout',
            'SoloAssistPolicy',
            'ContentEra',
            'ContentTier',
            'AdaptiveEncounterAPI',
            'ItemBudgetScaler',
            'GeneratedContentCensus',
            'InstanceProfile',
        ):
            shutil.copyfile(ROOT / f'include/{name}.h', out / f'{name}.h')

        for name in (
            'CombatBudgetProfile.cpp',
            'InstanceScaleContext.cpp',
            'InstanceProfile.cpp',
            'ContentPackRegistry.cpp',
            'ProgressionLayout.cpp',
            'GeneratedItemProfiles.cpp',
        ):
            shutil.copyfile(ROOT / f'src/{name}', out / name)

        (out / 'Define.h').write_text('''#pragma once
#include <cstdint>
using int8 = std::int8_t;
using uint8 = std::uint8_t;
using int16 = std::int16_t;
using uint16 = std::uint16_t;
using int32 = std::int32_t;
using uint32 = std::uint32_t;
using int64 = std::int64_t;
using uint64 = std::uint64_t;
''', encoding='utf-8')

        (out / 'DBCEnums.h').write_text('''#pragma once
#include "Define.h"
enum Difficulty : uint8
{
    REGULAR_DIFFICULTY           = 0,
    DUNGEON_DIFFICULTY_NORMAL    = 0,
    DUNGEON_DIFFICULTY_HEROIC    = 1,
    DUNGEON_DIFFICULTY_EPIC      = 2,
    RAID_DIFFICULTY_10MAN_NORMAL = 0,
    RAID_DIFFICULTY_25MAN_NORMAL = 1,
    RAID_DIFFICULTY_10MAN_HEROIC = 2,
    RAID_DIFFICULTY_25MAN_HEROIC = 3
};
constexpr uint8 MAX_RAID_DIFFICULTY = 4;
''', encoding='utf-8')

        (out / 'ObjectGuid.h').write_text('''#pragma once
#include "Define.h"
#include <functional>
#include <string>

struct ObjectGuid
{
    uint64 value = 0;
    constexpr ObjectGuid() = default;
    constexpr explicit ObjectGuid(uint64 v) : value(v) { }
    static ObjectGuid const Empty;
    bool IsEmpty() const { return value == 0; }
    bool IsGroup() const { return (value & (1ULL << 62)) != 0; }
    uint32 GetCounter() const { return static_cast<uint32>(value & 0xFFFFFFFFULL); }
    std::string ToString() const { return std::to_string(value); }
    explicit operator bool() const { return value != 0; }
    bool operator==(ObjectGuid const& other) const { return value == other.value; }
    bool operator!=(ObjectGuid const& other) const { return value != other.value; }
    bool operator<(ObjectGuid const& other) const { return value < other.value; }
};
inline constexpr ObjectGuid ObjectGuid::Empty{};

namespace std
{
    template<>
    struct hash<ObjectGuid>
    {
        size_t operator()(ObjectGuid const& guid) const noexcept
        {
            return std::hash<uint64>{}(guid.value);
        }
    };
}
''', encoding='utf-8')

        lfg_header = '''#pragma once
#include "Define.h"
#include "ObjectGuid.h"
#include <map>

namespace lfg
{
    enum class LfgCompositionMode : uint8
    {
        MATCHMAKING   = 0,
        BOT_FILL      = 1,
        CURRENT_PARTY = 2
    };

    struct LfgQueuePolicy
    {
        LfgCompositionMode compositionMode = LfgCompositionMode::BOT_FILL;
        uint32 challengeSize = 0;
        bool bypassMatchmaking = false;
        bool requireStandardRoles = true;
        uint8 minPlayers = 5;
        uint8 targetPlayers = 5;
    };

    struct LfgProposalPlayer {};

    struct LfgProposal
    {
        uint32 dungeonId = 0;
        LfgQueuePolicy policy{};
        std::map<ObjectGuid, LfgProposalPlayer> players;
    };

    struct LFGDungeonData
    {
        uint32 id = 0;
        uint32 map = 0;
        uint8 type = 1;
        uint8 minlevel = 1;
        uint8 maxlevel = 80;
    };
}
'''
        (out / 'LFG.h').write_text(lfg_header, encoding='utf-8')
        (out / 'DungeonFinding').mkdir(exist_ok=True)
        (out / 'DungeonFinding/LFG.h').write_text('#pragma once\n#include "../LFG.h"\n', encoding='utf-8')

        (out / 'LFGMgr.h').write_text('''#pragma once
#include "LFG.h"
#include <unordered_map>

struct LFGMgrStub
{
    std::unordered_map<uint32, lfg::LFGDungeonData> dungeons;
    lfg::LFGDungeonData const* GetLFGDungeon(uint32 id) const
    {
        auto it = dungeons.find(id);
        return it != dungeons.end() ? &it->second : nullptr;
    }
};
inline LFGMgrStub gLfgMgr;
#define sLFGMgr (&gLfgMgr)
''', encoding='utf-8')

        (out / 'CreatureData.h').write_text('''#pragma once
#include "Define.h"
#include <string>

enum CreatureEliteType : uint8
{
    CREATURE_ELITE_NORMAL     = 0,
    CREATURE_ELITE_ELITE      = 1,
    CREATURE_ELITE_RAREELITE  = 2,
    CREATURE_ELITE_WORLDBOSS  = 3,
    CREATURE_ELITE_RARE       = 4
};

enum CreatureFlagsExtra : uint32
{
    CREATURE_FLAG_EXTRA_INSTANCE_BIND = 0x00000001,
    CREATURE_FLAG_EXTRA_DUNGEON_BOSS  = 0x10000000
};

struct CreatureTemplate
{
    uint32 Entry = 0;
    std::string Name;
    uint8 minlevel = 1;
    uint8 maxlevel = 1;
    uint8 expansion = 0;
    uint8 unit_class = 1;
    uint8 rank = CREATURE_ELITE_NORMAL;
    uint32 flags_extra = 0;
    float ModHealth = 1.0f;
    float ModMana = 1.0f;
    float ModArmor = 1.0f;
    float DamageModifier = 1.0f;
};

struct CreatureBaseStats
{
    uint32 BaseHealth[3] = { 100, 100, 100 };
    uint32 BaseMana = 0;
    uint32 AttackPower = 10;
    float BaseDamage[3] = { 10.0f, 10.0f, 10.0f };
    float BaseArmor = 100.0f;
};
''', encoding='utf-8')

        (out / 'Player.h').write_text('''#pragma once
#include "Define.h"
#include "ObjectGuid.h"

class Map;
class Group;

class Player
{
public:
    ObjectGuid guid;
    uint8 level = 19;
    bool gm = false;
    bool beingTeleported = false;
    bool inWorld = true;
    bool isBot = false;
    Map* map = nullptr;
    Group* group = nullptr;

    ObjectGuid GetGUID() const { return guid; }
    uint8 GetLevel() const { return level; }
    bool IsGameMaster() const { return gm; }
    bool IsBeingTeleported() const { return beingTeleported; }
    bool IsInWorld() const { return inWorld; }
    Map* FindMap() const { return map; }
    Map* GetMap() const { return map; }
    Group* GetGroup() const { return group; }
};
''', encoding='utf-8')

        (out / 'Group.h').write_text('''#pragma once
#include "Define.h"
#include "ObjectGuid.h"

class Group
{
public:
    ObjectGuid guid;
    ObjectGuid leaderGuid;
    uint8 membersCount = 1;

    ObjectGuid GetGUID() const { return guid; }
    ObjectGuid GetLeaderGUID() const { return leaderGuid; }
    uint8 GetMembersCount() const { return membersCount; }
};
''', encoding='utf-8')

        (out / 'GroupMgr.h').write_text('''#pragma once
#include "Group.h"
#include <unordered_map>

struct GroupMgrStub
{
    std::unordered_map<uint32, Group*> groups;
    Group* GetGroupByGUID(uint32 counter) const
    {
        auto it = groups.find(counter);
        return it != groups.end() ? it->second : nullptr;
    }
};
inline GroupMgrStub gGroupMgr;
#define sGroupMgr (&gGroupMgr)
''', encoding='utf-8')

        (out / 'Map.h').write_text('''#pragma once
#include "DBCEnums.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <functional>
#include <unordered_map>
#include <vector>

class Player;
class Creature;

struct StubCreatureStore
{
    struct Elements
    {
        struct Inner
        {
            std::unordered_map<ObjectGuid, Creature*> _element;
        } _elements;
    } _data;
    Elements& GetElements() { return _data; }
};

class Map
{
public:
    uint32 mapId = 36;
    uint32 instanceId = 1;
    uint8 spawnMode = 0;
    uint32 maxPlayers = 5;
    bool dungeon = true;
    bool raid = false;
    bool heroic = false;
    std::vector<Player*> players;
    StubCreatureStore objectsStore;

    uint32 GetId() const { return mapId; }
    uint32 GetInstanceId() const { return instanceId; }
    uint8 GetSpawnMode() const { return spawnMode; }
    Difficulty GetDifficulty() const { return static_cast<Difficulty>(spawnMode); }
    uint32 GetMaxPlayers() const { return maxPlayers; }
    bool IsDungeon() const { return dungeon; }
    bool IsRaid() const { return raid; }
    bool IsHeroic() const { return heroic; }

    void DoForAllPlayers(std::function<void(Player*)> const& exec)
    {
        for (Player* p : players)
            exec(p);
    }

    StubCreatureStore& GetObjectsStore() { return objectsStore; }

    Creature* GetCreature(ObjectGuid const& guid)
    {
        auto it = objectsStore._data._elements._element.find(guid);
        return it != objectsStore._data._elements._element.end() ? it->second : nullptr;
    }
};

class InstanceMap : public Map {};
''', encoding='utf-8')

        (out / 'Creature.h').write_text('''#pragma once
#include "CreatureData.h"
#include "Map.h"
#include "ObjectGuid.h"

enum UnitMods
{
    UNIT_MOD_STAT_STRENGTH = 0,
    UNIT_MOD_HEALTH,
    UNIT_MOD_MANA,
    UNIT_MOD_ARMOR,
    UNIT_MOD_ATTACK_POWER
};

enum UnitModifierType
{
    BASE_VALUE = 0,
    BASE_PCT   = 1,
    TOTAL_VALUE = 2,
    TOTAL_PCT  = 3
};

enum Powers
{
    POWER_MANA = 0
};

enum WeaponAttackType
{
    BASE_ATTACK   = 0,
    OFF_ATTACK    = 1,
    RANGED_ATTACK = 2
};

enum WeaponDamageRange
{
    MINDAMAGE = 0,
    MAXDAMAGE = 1
};

class Creature
{
public:
    ObjectGuid guid{100};
    CreatureTemplate const* cinfo = nullptr;
    Map* map = nullptr;
    uint8 level = 19;
    uint32 areaId = 0;
    float scalingMultiplier = 1.0f;

    bool inWorld = true;
    bool alive = true;
    bool inCombat = false;
    bool inEvadeMode = false;
    bool isPet = false;
    bool isTotem = false;
    bool isCritter = false;
    bool isTrigger = false;
    bool controlledByPlayer = false;

    uint32 createHealth = 100;
    uint32 maxHealth = 100;
    uint32 curHealth = 100;
    float baseHealthMod = 100.0f;
    float totalHealthPctMod = 1.0f; // Ensures Aura/Percentage modifiers are tested if any

    uint32 createMana = 0;
    uint32 maxMana = 0;
    uint32 curMana = 0;
    float baseManaMod = 0.0f;
    float totalManaPctMod = 1.0f;
    float armor = 0.0f;
    uint32 attackPower = 0;
    float minDamage = 0.0f;
    float maxDamage = 0.0f;
    uint32 playerDamageReq = 100;

    Creature() = default;
    Creature(CreatureTemplate const* tpl, Map* m, uint64 guidVal = 5001)
        : guid(guidVal), cinfo(tpl), map(m), level(tpl ? tpl->maxlevel : 19), scalingMultiplier(2.2f)
    {
        if (map)
            map->objectsStore._data._elements._element[guid] = this;
    }
    ~Creature()
    {
        if (map)
        {
            auto& elems = map->objectsStore._data._elements._element;
            auto it = elems.find(guid);
            if (it != elems.end() && it->second == this)
                elems.erase(it);
        }
    }

    ObjectGuid GetGUID() const { return guid; }
    uint32 GetEntry() const { return cinfo ? cinfo->Entry : 0; }
    CreatureTemplate const* GetCreatureTemplate() const { return cinfo; }
    Map* GetMap() const { return map; }
    uint8 GetLevel() const { return level; }
    uint32 GetAreaId() const { return areaId; }

    bool IsInWorld() const { return inWorld; }
    bool IsAlive() const { return alive; }
    bool IsInCombat() const { return inCombat; }
    bool IsInEvadeMode() const { return inEvadeMode; }
    bool IsPet() const { return isPet; }
    bool IsTotem() const { return isTotem; }
    bool IsCritter() const { return isCritter; }
    bool IsTrigger() const { return isTrigger; }
    bool IsControlledByPlayer() const { return controlledByPlayer; }

    uint32 GetMaxHealth() const { return maxHealth; }
    uint32 GetHealth() const { return curHealth; }
    float GetHealthPct() const { return maxHealth ? (100.0f * float(curHealth) / float(maxHealth)) : 0.0f; }

    void SetCreateHealth(uint32 h)
    {
        createHealth = h;
        if (map)
            map->objectsStore._data._elements._element[guid] = this;
    }
    void SetStatFlatModifier(UnitMods mod, UnitModifierType type, float val)
    {
        if (mod == UNIT_MOD_HEALTH && type == BASE_VALUE)
            baseHealthMod = val;
        else if (mod == UNIT_MOD_MANA && type == BASE_VALUE)
            baseManaMod = val;
        else if (mod == UNIT_MOD_ARMOR && type == BASE_VALUE)
            armor = val;
        else if (mod == UNIT_MOD_ATTACK_POWER && type == BASE_VALUE)
            attackPower = static_cast<uint32>(val);
    }
    void UpdateMaxHealth()
    {
        maxHealth = static_cast<uint32>(baseHealthMod * totalHealthPctMod);
    }
    void SetHealth(uint32 h)
    {
        curHealth = (h > maxHealth) ? maxHealth : h;
    }

    uint32 GetPlayerDamageReq() const { return playerDamageReq; }
    void LowerPlayerDamageReq(uint32 amount, bool)
    {
        playerDamageReq = (amount >= playerDamageReq) ? 0 : (playerDamageReq - amount);
    }
    void ResetPlayerDamageReq()
    {
        playerDamageReq = maxHealth / 2;
    }

    void SetCreateMana(uint32 m) { createMana = m; }
    uint32 GetMaxPower(Powers) const { return maxMana; }
    uint32 GetPower(Powers) const { return curMana; }
    void SetMaxPower(Powers, uint32 m) { maxMana = m; }
    void SetPower(Powers, uint32 m) { curMana = (m > maxMana) ? maxMana : m; }
    void UpdateMaxPower(Powers)
    {
        maxMana = static_cast<uint32>(baseManaMod * totalManaPctMod);
    }
    void UpdateArmor() {}
    void UpdateAttackPowerAndDamage() {}
    void SetBaseWeaponDamage(WeaponAttackType, WeaponDamageRange range, float val)
    {
        if (range == MINDAMAGE)
            minDamage = val;
        else
            maxDamage = val;
    }
    void UpdateDamagePhysical(WeaponAttackType) {}
};
''', encoding='utf-8')

        (out / 'ObjectMgr.h').write_text('''#pragma once
#include "CreatureData.h"
#include <map>
#include <utility>

struct ObjectMgrStub
{
    std::map<std::pair<uint8, uint8>, CreatureBaseStats> statsByLevelClass;
    CreatureBaseStats fallbackStats;

    ObjectMgrStub()
    {
        // Rhahk'Zor (L19 class 1) & high-level scaled viewer (L46, L49 class 1) & Lord Cobrahn (L20 class 1)
        CreatureBaseStats s19;
        s19.BaseHealth[0] = 449;
        s19.BaseDamage[0] = 17.5f;
        s19.BaseArmor = 750.0f;
        statsByLevelClass[{19, 1}] = s19;

        CreatureBaseStats s20;
        s20.BaseHealth[0] = 484;
        s20.BaseDamage[0] = 18.5f;
        s20.BaseArmor = 800.0f;
        statsByLevelClass[{20, 1}] = s20;

        CreatureBaseStats s46;
        s46.BaseHealth[0] = 1919;
        statsByLevelClass[{46, 1}] = s46;

        CreatureBaseStats s49;
        s49.BaseHealth[0] = 2138;
        statsByLevelClass[{49, 1}] = s49;

        CreatureBaseStats s80;
        s80.BaseHealth[0] = 12000;
        s80.BaseHealth[1] = 12000;
        s80.BaseHealth[2] = 12000;
        statsByLevelClass[{80, 1}] = s80;
    }

    CreatureBaseStats const* GetCreatureBaseStats(uint8 level, uint8 unitClass) const
    {
        auto it = statsByLevelClass.find({level, unitClass});
        return it != statsByLevelClass.end() ? &it->second : &fallbackStats;
    }
};
inline ObjectMgrStub gObjectMgr;
#define sObjectMgr (&gObjectMgr)
''', encoding='utf-8')

        (out / 'ObjectAccessor.h').write_text('''#pragma once
#include "Player.h"
#include <unordered_map>

namespace ObjectAccessor
{
    inline std::unordered_map<ObjectGuid, Player*> players;
    inline Player* FindPlayer(ObjectGuid const& guid)
    {
        auto it = players.find(guid);
        return it != players.end() ? it->second : nullptr;
    }
}
''', encoding='utf-8')

        (out / 'CoACreatureScaling.h').write_text('''#pragma once
#include "Creature.h"
namespace CreatureScaling
{
    inline float HealthMultiplier(Creature const* creature)
    {
        return creature ? creature->scalingMultiplier : 1.0f;
    }
}
''', encoding='utf-8')

        (out / 'CoACreatureScalingPolicy.h').write_text('''#pragma once
#include "Define.h"
namespace CreatureScaling
{
    inline uint32 Scaled(uint32 value, float multiplier)
    {
        return static_cast<uint32>(double(value) * multiplier + 0.5);
    }
}
''', encoding='utf-8')

        (out / 'World.h').write_text('''#pragma once
enum ServerConfigs
{
    RATE_CREATURE_NORMAL_HP = 0,
    RATE_CREATURE_ELITE_ELITE_HP,
    RATE_CREATURE_ELITE_RAREELITE_HP,
    RATE_CREATURE_ELITE_WORLDBOSS_HP,
    RATE_CREATURE_ELITE_RARE_HP
};
struct WorldStub
{
    float rates[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    float getRate(ServerConfigs r) const { return rates[r]; }
};
inline WorldStub gWorld;
#define sWorld (&gWorld)
''', encoding='utf-8')

        for empty_hdr in ('DatabaseEnv.h', 'Field.h', 'QueryResult.h', 'Log.h'):
            if empty_hdr == 'DatabaseEnv.h':
                (out / empty_hdr).write_text('''#pragma once
#include "QueryResult.h"
struct DatabaseStub
{
    QueryResult Query(char const*) { return nullptr; }
    template<typename... Args>
    void Execute(char const*, Args&&...) {}
};
inline DatabaseStub WorldDatabase;
inline DatabaseStub CharacterDatabase;
''', encoding='utf-8')
            elif empty_hdr == 'Field.h':
                (out / empty_hdr).write_text('''#pragma once
#include "Define.h"
struct Field
{
    template<typename T> T Get() const { return T{}; }
};
''', encoding='utf-8')
            elif empty_hdr == 'QueryResult.h':
                (out / empty_hdr).write_text('''#pragma once
#include "Field.h"
#include <memory>
struct ResultSetStub
{
    Field* Fetch() { return nullptr; }
    bool NextRow() { return false; }
};
using QueryResult = std::shared_ptr<ResultSetStub>;
''', encoding='utf-8')
            elif empty_hdr == 'Log.h':
                (out / empty_hdr).write_text('''#pragma once
#define LOG_INFO(...) do {} while (0)
#define LOG_WARN(...) do {} while (0)
#define LOG_DEBUG(...) do {} while (0)
#define LOG_ERROR(...) do {} while (0)
#define LOG_FATAL(...) do {} while (0)
''', encoding='utf-8')

        (out / 'GameTime.h').write_text('''#pragma once
#include <chrono>
namespace GameTime
{
    inline std::chrono::seconds GetGameTime() { return std::chrono::seconds(1000); }
}
''', encoding='utf-8')

        (out / 'ScriptMgr.h').write_text('''#pragma once
struct ScriptMgrStub
{
    bool hasBotFillProvider = true;
    bool HasLfgAutoFillProvider() const { return hasBotFillProvider; }
};
inline ScriptMgrStub gScriptMgr;
#define sScriptMgr (&gScriptMgr)
''', encoding='utf-8')

        (out / 'CoAContentScalingStub.cpp').write_text(f'''#include "CoAContentScaling.h"
#include "CombatBudgetProfile.h"
#include "ContentPackRegistry.h"
#include "Creature.h"
#include "CreatureData.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Group.h"
#include "GroupMgr.h"
#include "InstanceScaleContext.h"
#include "LFGMgr.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SoloAssistPolicy.h"
#include <algorithm>
#include <cmath>

void CoAContentScaling::SavePlayerLfgSettings(Player*) {{}}

{extracted_methods}
''', encoding='utf-8')

        (out / 'test_scenarios.cpp').write_text('''#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include "CoAContentScaling.h"
#include "CoACreatureScalingPolicy.h"
#include "CombatBudgetProfile.h"
#include "ContentPackRegistry.h"
#include "Creature.h"
#include "CreatureData.h"
#include "Group.h"
#include "GroupMgr.h"
#include "InstanceScaleContext.h"
#include "LFGMgr.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace
{
    CreatureTemplate MakeRhahkZorTemplate()
    {
        CreatureTemplate t;
        t.Entry = 644;
        t.Name = "Rhahk'Zor";
        t.minlevel = 19;
        t.maxlevel = 19;
        t.expansion = 0;
        t.unit_class = 1;
        t.rank = CREATURE_ELITE_ELITE;
        t.flags_extra = CREATURE_FLAG_EXTRA_DUNGEON_BOSS;
        t.ModHealth = 10.0f;
        t.DamageModifier = 1.0f;
        return t;
    }

    CreatureTemplate MakeCobrahnTemplate()
    {
        CreatureTemplate t;
        t.Entry = 3669;
        t.Name = "Lord Cobrahn";
        t.minlevel = 20;
        t.maxlevel = 20;
        t.expansion = 0;
        t.unit_class = 1;
        t.rank = CREATURE_ELITE_ELITE;
        t.flags_extra = CREATURE_FLAG_EXTRA_DUNGEON_BOSS;
        t.ModHealth = 8.0f;
        t.DamageModifier = 1.0f;
        return t;
    }

    #define MakeBoss Creature

    uint32 ComputeDestinyWeaverViewMaxHealth(Creature const& boss, uint8 viewerLevel)
    {
        auto const* ownStats = sObjectMgr->GetCreatureBaseStats(boss.GetLevel(), boss.GetCreatureTemplate()->unit_class);
        auto const* viewStats = sObjectMgr->GetCreatureBaseStats(viewerLevel, boss.GetCreatureTemplate()->unit_class);
        double const ratio = double(viewStats->BaseHealth[0]) / double(ownStats->BaseHealth[0]);
        return static_cast<uint32>(std::round(double(boss.GetMaxHealth()) * ratio));
    }

    void ResetState()
    {
        sInstanceScalingMgr->Clear();
        ObjectAccessor::players.clear();
        gGroupMgr.groups.clear();
        gLfgMgr.dungeons.clear();
        sCoAContentScaling->SetGroupScalingEnabled(true);
        sCoAContentScaling->SetLockOnEncounterStart(true);
    }
}

int main()
{
    auto const rhahkZorTpl = MakeRhahkZorTemplate();
    auto const cobrahnTpl = MakeCobrahnTemplate();

    // =========================================================================
    // Scenario 1: Rhahk'Zor, five players -> Consistent spawn/pull HP
    // =========================================================================
    {
        ResetState();
        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 101;
        Player p1{ObjectGuid(1), 19}, p2{ObjectGuid(2), 19}, p3{ObjectGuid(3), 19}, p4{ObjectGuid(4), 19}, p5{ObjectGuid(5), 19};
        deadmines.players = { &p1, &p2, &p3, &p4, &p5 };

        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &boss);

        uint32 const spawnHp = boss.GetMaxHealth();
        assert(spawnHp == 26671);
        assert(boss.GetHealth() == 26671);

        // Pull boss
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);

        uint32 const pullHp = boss.GetMaxHealth();
        assert(pullHp == spawnHp);
        assert(boss.GetHealth() == pullHp);
    }

    // =========================================================================
    // Scenario 2: Rhahk'Zor, solo player -> Correct solo health budget
    // =========================================================================
    {
        ResetState();
        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 102;
        Player solo{ObjectGuid(1), 19};
        deadmines.players = { &solo };

        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &boss);

        uint32 const soloSpawnHp = boss.GetMaxHealth();
        assert(soloSpawnHp == 5782);

        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);

        assert(boss.GetMaxHealth() == soloSpawnHp);
        assert(boss.GetHealth() == soloSpawnHp);
    }

    // =========================================================================
    // Scenario 3 & 4: Rhahk'Zor, one human + four bots (including mixed-level L49 bots)
    // Reproduces the exact user bug where Rhahk'Zor spawned while 0-1 players were
    // registered in m_mapRefMgr (~25k L49 view HP = 5,782 server HP) and then 4 bots
    // entered the map. With RefreshDungeonCreatures on OnPlayerEnterAll, Rhahk'Zor
    // updates to 5-player HP (~114k-115k L49 view HP = 26,671 server HP) BEFORE combat
    // and does NOT spike on combat start!
    // =========================================================================
    {
        ResetState();
        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 103;
        sInstanceScalingMgr->SetCompositionMode(36, 103, lfg::LfgCompositionMode::CURRENT_PARTY);

        // Step A: Grid loads while first player is zoning in (before AddToWorld adds to m_mapRefMgr)
        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &boss);
        assert(boss.GetMaxHealth() == 5782);
        // Verify that at 1-player server HP (5,782), a level 46-49 viewer sees ~24.7k-27.5k HP
        uint32 const view1pL46 = ComputeDestinyWeaverViewMaxHealth(boss, 46);
        uint32 const view1pL49 = ComputeDestinyWeaverViewMaxHealth(boss, 49);
        assert(view1pL46 == 24712);
        assert(view1pL49 == 27532);

        // Step B: Human player and 4 mixed-level playerbots (including level 49 bots) finish entering map
        Player human{ObjectGuid(1), 19};
        Player bot1{ObjectGuid(2), 22, false, false, true, true};
        Player bot2{ObjectGuid(3), 35, false, false, true, true};
        Player bot3{ObjectGuid(4), 46, false, false, true, true};
        Player bot4{ObjectGuid(5), 49, false, false, true, true};

        for (Player* member : { &human, &bot1, &bot2, &bot3, &bot4 })
        {
            deadmines.players.push_back(member);
            // OnPlayerEnterAll hook refreshes out-of-combat dungeon creatures
            sCoAContentScaling->RefreshDungeonCreatures(&deadmines);
        }

        // Before combat starts, Rhahk'Zor is already at full 5-participant budget!
        uint32 const prePullHp = boss.GetMaxHealth();
        assert(prePullHp == 26671);
        assert(boss.GetHealth() == 26671);
        assert(ComputeDestinyWeaverViewMaxHealth(boss, 46) == 113990);
        assert(ComputeDestinyWeaverViewMaxHealth(boss, 49) == 126999);

        // Verify that high-level bots do NOT add any extra level multiplier to server boss HP
        assert(prePullHp == 26671);

        // Step C: Enter combat -> zero HP spike!
        boss.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);
        assert(boss.GetMaxHealth() == prePullHp);
        assert(boss.GetHealth() == prePullHp);
    }

    // =========================================================================
    // Scenario 5: Wailing Caverns boss (Lord Cobrahn) -> Consistent spawn/pull HP
    // =========================================================================
    {
        ResetState();
        Map wailingCaverns;
        wailingCaverns.mapId = 43;
        wailingCaverns.instanceId = 105;
        Player p1{ObjectGuid(1), 20}, p2{ObjectGuid(2), 20}, p3{ObjectGuid(3), 20}, p4{ObjectGuid(4), 49}, p5{ObjectGuid(5), 49};
        Creature cobrahn = MakeBoss(&cobrahnTpl, &wailingCaverns, 5002);

        // Spawn before players are in m_mapRefMgr, then players enter
        sCoAContentScaling->ApplyCreatureScaling(&cobrahnTpl, &cobrahn);
        for (Player* p : { &p1, &p2, &p3, &p4, &p5 })
        {
            wailingCaverns.players.push_back(p);
            sCoAContentScaling->RefreshDungeonCreatures(&wailingCaverns);
        }

        uint32 const prePullHp = cobrahn.GetMaxHealth();
        assert(prePullHp == 23001);
        assert(cobrahn.GetHealth() == prePullHp);

        cobrahn.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &wailingCaverns,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, cobrahn.GetEntry()},
            &cobrahn);

        assert(cobrahn.GetMaxHealth() == prePullHp);
        assert(cobrahn.GetHealth() == prePullHp);
    }

    // =========================================================================
    // Scenario 6: Group scaling disabled (GroupScaling.Enable = 0)
    // No group scaling applied at spawn or at pull, even for solo or partial groups!
    // =========================================================================
    {
        ResetState();
        sCoAContentScaling->SetGroupScalingEnabled(false);

        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 106;
        Player solo{ObjectGuid(1), 19};
        deadmines.players = { &solo };

        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &boss);

        // With GroupScaling.Enable = 0, healthScale is 1.0f (full 5-player dungeon budget = 26,671)
        uint32 const spawnHp = boss.GetMaxHealth();
        assert(spawnHp == 26671);

        InstanceScaleContext const ctx = sInstanceScalingMgr->GetOrCreateContext(&deadmines);
        assert(ctx.healthScale == 1.0f);
        assert(ctx.damageScale == 1.0f);

        // Even if BeginEncounter / RecalculateEncounterCombatStats is invoked, HP never changes
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);
        assert(boss.GetMaxHealth() == spawnHp);

        EncounterScaleSnapshot fakeSnap;
        fakeSnap.healthScale = 0.21674f;
        fakeSnap.damageScale = 0.28f;
        fakeSnap.effectivePlayers = 1.0f;
        fakeSnap.valid = true;
        sCoAContentScaling->RecalculateEncounterCombatStats(&boss, fakeSnap, EncounterHealthTransferPolicy::FULL_ON_PULL);
        assert(boss.GetMaxHealth() == spawnHp);
    }

    auto ExpectedRhahkZorHp = [](uint32 effectivePlayers) -> uint32
    {
        float const ratio = std::clamp<float>(float(effectivePlayers) / 5.0f, 0.025f, 1.0f);
        float const healthScale = std::max<float>(0.05f, std::pow(ratio, 0.95f));
        float finalHealth = 449.0f * 10.0f * 1.35f * healthScale * 2.0f;
        return CreatureScaling::Scaled(std::max<uint32>(1, static_cast<uint32>(std::ceil(finalHealth))), 2.2f);
    };

    // =========================================================================
    // Scenario 7: Group scaling enabled -> Correct effective-player multiplier (1..5 players)
    // =========================================================================
    {
        ResetState();
        uint32 prevHp = 0;
        for (uint32 count = 1; count <= 5; ++count)
        {
            sInstanceScalingMgr->Clear();
            Map deadmines;
            deadmines.mapId = 36;
            deadmines.instanceId = 1070 + count;
            std::vector<Player> party(count);
            for (uint32 i = 0; i < count; ++i)
            {
                party[i].guid = ObjectGuid(i + 1);
                deadmines.players.push_back(&party[i]);
            }

            Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
            sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &boss);
            assert(boss.GetMaxHealth() == ExpectedRhahkZorHp(count));
            assert(boss.GetMaxHealth() > prevHp);
            prevHp = boss.GetMaxHealth();
        }
        assert(ExpectedRhahkZorHp(1) == 5782);
        assert(ExpectedRhahkZorHp(5) == 26671);
    }

    // =========================================================================
    // Scenario 8: Fixed challenge size -> Correct virtual difficulty
    // =========================================================================
    {
        ResetState();
        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 108;
        Player solo{ObjectGuid(1), 19};
        solo.map = &deadmines;
        ObjectAccessor::players[solo.guid] = &solo;
        deadmines.players = { &solo };

        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &boss);
        assert(boss.GetMaxHealth() == 5782);

        // Solo player sets fixed challenge size = 5 while inside the dungeon
        assert(sCoAContentScaling->SetPlayerLfgChallenge(solo.guid, 5));
        assert(boss.GetMaxHealth() == 26671);

        // Entering combat preserves the fixed challenge size 5 budget
        boss.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);
        assert(boss.GetMaxHealth() == 26671);
    }

    // =========================================================================
    // Scenario 9: Adaptive challenge size (0) -> Uses appropriate participant count
    // =========================================================================
    {
        ResetState();
        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 109;
        Player p1{ObjectGuid(1), 19}, p2{ObjectGuid(2), 19}, p3{ObjectGuid(3), 19};
        deadmines.players = { &p1, &p2, &p3 };

        sInstanceScalingMgr->SetChallengeSize(36, 109, 0);
        sInstanceScalingMgr->SetCompositionMode(36, 109, lfg::LfgCompositionMode::CURRENT_PARTY);

        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &boss);
        assert(boss.GetMaxHealth() == ExpectedRhahkZorHp(3)); // 3-player budget
    }

    // =========================================================================
    // Scenario 10: Repeated encounter callbacks -> No duplicate scaling or mid-fight re-lock
    // =========================================================================
    {
        ResetState();
        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 110;
        Player p1{ObjectGuid(1), 19}, p2{ObjectGuid(2), 19}, p3{ObjectGuid(3), 19}, p4{ObjectGuid(4), 19}, p5{ObjectGuid(5), 19};
        deadmines.players = { &p1, &p2, &p3, &p4, &p5 };

        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &boss);
        assert(boss.GetMaxHealth() == 26671);

        // 1) OnUnitEnterCombat locks encounter (CREATURE_FALLBACK)
        boss.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);
        uint32 const gen1 = sInstanceScalingMgr->GetContext(36, 110).activeSnapshot.generation;
        assert(gen1 == 1);

        // Boss takes damage mid-fight (down to 10,000 HP)
        boss.SetHealth(10000);

        // 2) InstanceScript SetBossState(IN_PROGRESS) fires with boss == nullptr
        // Must upgrade source to INSTANCE_SCRIPT WITHOUT re-locking or incrementing generation!
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::INSTANCE_SCRIPT,
            EncounterKey{EncounterKeyType::INSTANCE_ENCOUNTER, 1},
            nullptr);
        InstanceScaleContext const ctxAfterUpgrade = sInstanceScalingMgr->GetContext(36, 110);
        assert(ctxAfterUpgrade.encounterLocked);
        assert(ctxAfterUpgrade.activeEncounterSource == EncounterLifecycleSource::INSTANCE_SCRIPT);
        assert(ctxAfterUpgrade.activeSnapshot.generation == gen1);

        // 3) Repeated OnUnitEnterCombat on the same boss must NOT heal the boss back to full or re-scale!
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);
        assert(boss.GetMaxHealth() == 26671);
        assert(boss.GetHealth() == 10000);

        // 4) Even when LockOnEncounterStart = 0, active encounter state is tracked so repeated callbacks mid-fight do not heal boss
        ResetState();
        sCoAContentScaling->SetLockOnEncounterStart(false);
        deadmines.players = { &p1, &p2, &p3, &p4, &p5 };
        Creature bossUnlocked = MakeBoss(&rhahkZorTpl, &deadmines, 5010);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &bossUnlocked);
        bossUnlocked.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::INSTANCE_SCRIPT,
            EncounterKey{EncounterKeyType::INSTANCE_ENCOUNTER, 1},
            nullptr);
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, bossUnlocked.GetEntry()},
            &bossUnlocked);
        bossUnlocked.SetHealth(12000);
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, bossUnlocked.GetEntry()},
            &bossUnlocked);
        assert(bossUnlocked.GetHealth() == 12000);
    }

    // =========================================================================
    // Scenario 11: Boss evade/reset -> Correct health restoration and scaling state
    // =========================================================================
    {
        ResetState();
        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 111;
        Player p1{ObjectGuid(1), 19}, p2{ObjectGuid(2), 19}, p3{ObjectGuid(3), 19}, p4{ObjectGuid(4), 19}, p5{ObjectGuid(5), 19};
        deadmines.players = { &p1, &p2, &p3, &p4, &p5 };

        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &boss);

        boss.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);
        boss.SetHealth(8000);

        // 4 players leave during the wipe, leaving only p1
        deadmines.players = { &p1 };
        // While locked in combat, boss stays at 5-player HP (26,671)
        sCoAContentScaling->RefreshDungeonCreatures(&deadmines);
        assert(boss.GetMaxHealth() == 26671);

        // Boss evades and exits combat -> lock released and boss rescales to solo HP (5,782) at full health
        boss.inCombat = false;
        boss.inEvadeMode = true;
        sInstanceScalingMgr->EndEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            EncounterLockState::RESETTING,
            boss.GetGUID());

        assert(!sInstanceScalingMgr->GetContext(36, 111).encounterLocked);
        assert(boss.GetMaxHealth() == 5782);
        assert(boss.GetHealth() == 5782);

        // Also verify stale INSTANCE_SCRIPT lock is cleared if all players leave the map (inWorld = false)
        boss.inEvadeMode = false;
        boss.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::INSTANCE_SCRIPT,
            EncounterKey{EncounterKeyType::INSTANCE_ENCOUNTER, 1},
            &boss);
        assert(sInstanceScalingMgr->GetContext(36, 111).encounterLocked);
        p1.inWorld = false; // Last player removed from world during RemovePlayerFromMap
        sInstanceScalingMgr->ResetEncounterLockIfMapEmpty(&deadmines);
        assert(!sInstanceScalingMgr->GetContext(36, 111).encounterLocked);
    }

    // =========================================================================
    // Scenario 12: Group changes before pull -> Consistent recalculation (no accumulation)
    // & m_mapRefMgr RemovePlayerFromMap order (inWorld = false before ResetMap unlink)
    // =========================================================================
    {
        ResetState();
        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 112;
        Player p1{ObjectGuid(1), 19}, p2{ObjectGuid(2), 19}, p3{ObjectGuid(3), 19}, p4{ObjectGuid(4), 19}, p5{ObjectGuid(5), 19};

        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
        // Verify even with a 1.5x aura percentage modifier on the creature, repeated rescales never accumulate!
        boss.totalHealthPctMod = 1.5f;

        deadmines.players = { &p1 };
        sCoAContentScaling->RefreshDungeonCreatures(&deadmines);
        uint32 const soloMax = boss.GetMaxHealth();
        assert(soloMax == static_cast<uint32>(5782 * 1.5f));
        assert(boss.GetPlayerDamageReq() == soloMax / 2);

        deadmines.players = { &p1, &p2, &p3, &p4, &p5 };
        for (int i = 0; i < 10; ++i)
            sCoAContentScaling->RefreshDungeonCreatures(&deadmines);
        uint32 const fiveMax = boss.GetMaxHealth();
        assert(fiveMax == static_cast<uint32>(26671 * 1.5f));
        // Verify loot damage requirement scales up when uninjured creature scales up before pull
        assert(boss.GetPlayerDamageReq() == fiveMax / 2);

        // Simulate Map::RemovePlayerFromMap where p2..p5 have RemoveFromWorld() called (inWorld = false)
        // BEFORE ResetMap() unlinks them from m_mapRefMgr (so they are still in deadmines.players)
        p2.inWorld = false;
        p3.inWorld = false;
        p4.inWorld = false;
        p5.inWorld = false;
        sCoAContentScaling->RefreshDungeonCreatures(&deadmines);
        assert(boss.GetMaxHealth() == soloMax);
        assert(boss.GetPlayerDamageReq() == soloMax / 2);
    }

    // =========================================================================
    // Scenario 13: Group changes during fight -> Snapshot remains locked
    // =========================================================================
    {
        ResetState();
        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 113;
        Player p1{ObjectGuid(1), 19}, p2{ObjectGuid(2), 19};
        deadmines.players = { &p1, &p2 };

        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &boss);
        uint32 const twoPlayerHp = ExpectedRhahkZorHp(2);
        assert(boss.GetMaxHealth() == twoPlayerHp);

        boss.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);

        // 3 more players zone in mid-fight
        Player p3{ObjectGuid(3), 19}, p4{ObjectGuid(4), 19}, p5{ObjectGuid(5), 19};
        deadmines.players = { &p1, &p2, &p3, &p4, &p5 };
        sCoAContentScaling->RefreshDungeonCreatures(&deadmines);

        // Snapshot and boss HP remain locked at 2-player values
        InstanceScaleContext const lockedCtx = sInstanceScalingMgr->GetOrCreateContext(&deadmines);
        assert(lockedCtx.encounterLocked);
        assert(lockedCtx.actualParticipants == 2);
        assert(boss.GetMaxHealth() == twoPlayerHp);
    }

    // =========================================================================
    // Scenario 14: Encounter completion -> Lock released
    // =========================================================================
    {
        ResetState();
        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 114;
        Player p1{ObjectGuid(1), 19}, p2{ObjectGuid(2), 19};
        deadmines.players = { &p1, &p2 };

        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines);
        boss.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);
        assert(sInstanceScalingMgr->GetContext(36, 114).encounterLocked);

        boss.alive = false;
        boss.inCombat = false;
        sInstanceScalingMgr->EndEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            EncounterLockState::COMPLETED,
            boss.GetGUID());

        InstanceScaleContext const postCtx = sInstanceScalingMgr->GetContext(36, 114);
        assert(!postCtx.encounterLocked);
        assert(postCtx.lockState == EncounterLockState::COMPLETED);
    }

    // =========================================================================
    // Scenario 15: Multiple bosses / council -> No repeated or conflicting scaling
    // =========================================================================
    {
        ResetState();
        Map deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 115;
        Player p1{ObjectGuid(1), 19}, p2{ObjectGuid(2), 19}, p3{ObjectGuid(3), 19};
        deadmines.players = { &p1, &p2, &p3 };

        Creature council1 = MakeBoss(&rhahkZorTpl, &deadmines, 6001);
        Creature council2 = MakeBoss(&cobrahnTpl, &deadmines, 6002);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &council1);
        sCoAContentScaling->ApplyCreatureScaling(&cobrahnTpl, &council2);

        uint32 const c1Pre = council1.GetMaxHealth();
        uint32 const c2Pre = council2.GetMaxHealth();

        // Council member 1 enters combat
        council1.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, council1.GetEntry()},
            &council1);

        // Council member 2 enters combat
        council2.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, council2.GetEntry()},
            &council2);

        assert(council1.GetMaxHealth() == c1Pre);
        assert(council2.GetMaxHealth() == c2Pre);
        assert(sInstanceScalingMgr->GetContext(36, 115).activeSnapshot.generation == 1);

        // Council member 1 dies while council member 2 is still alive and in combat -> lock MUST stay active!
        council1.alive = false;
        council1.inCombat = false;
        sInstanceScalingMgr->EndEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, council1.GetEntry()},
            EncounterLockState::COMPLETED,
            council1.GetGUID());
        assert(sInstanceScalingMgr->GetContext(36, 115).encounterLocked);

        // Council member 2 dies -> now encounter unlocks!
        council2.alive = false;
        council2.inCombat = false;
        sInstanceScalingMgr->EndEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, council2.GetEntry()},
            EncounterLockState::COMPLETED,
            council2.GetGUID());
        assert(!sInstanceScalingMgr->GetContext(36, 115).encounterLocked);
    }

    // =========================================================================
    // Scenario 16: Raid calibrated HP (coa_boss_flex) -> Preserves calibrated boss budget
    // =========================================================================
    {
        ResetState();
        Map raidMap;
        raidMap.mapId = 533; // Naxxramas
        raidMap.instanceId = 116;
        raidMap.dungeon = true;
        raidMap.raid = true;
        raidMap.spawnMode = 0; // 10-man normal
        raidMap.maxPlayers = 10;

        CreatureTemplate patchwerk = rhahkZorTpl;
        patchwerk.Entry = 16028;
        patchwerk.minlevel = 80;
        patchwerk.maxlevel = 80;

        // Register calibrated boss flex: 500,000 HP per player on difficulty 0
        sInstanceScalingMgr->SetCalibratedBossFlexProfile(16028, 0, 500000);

        std::vector<Player> raiders(6);
        for (uint32 i = 0; i < 6; ++i)
        {
            raiders[i].guid = ObjectGuid(i + 1);
            raiders[i].level = 80;
            raidMap.players.push_back(&raiders[i]);
        }

        Creature boss = MakeBoss(&patchwerk, &raidMap, 7001);
        sCoAContentScaling->ApplyCreatureScaling(&patchwerk, &boss);

        // 6 players * 500,000 HP/player = 3,000,000 HP at spawn
        assert(boss.GetMaxHealth() == 3000000);

        // Pull boss -> still 3,000,000 HP (no spike!)
        boss.inCombat = true;
        sInstanceScalingMgr->BeginEncounter(
            &raidMap,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);
        assert(boss.GetMaxHealth() == 3000000);
    }

    // =========================================================================
    // Scenario 17: LFG BotFill with scaling, incomplete recruitment, bot disconnects,
    // opening-hit preservation, & GroupScaling.Enable = 0
    // =========================================================================
    {
        ResetState();
        Player leader{ObjectGuid(1), 19};
        Group group;
        group.guid = ObjectGuid((1ULL << 62) | 10);
        group.leaderGuid = leader.guid;
        group.membersCount = 1;
        leader.group = &group;
        ObjectAccessor::players[leader.guid] = &leader;
        gGroupMgr.groups[10] = &group;

        lfg::LFGDungeonData dmDungeon;
        dmDungeon.id = 30;
        dmDungeon.map = 36;
        gLfgMgr.dungeons[30] = dmDungeon;

        sCoAContentScaling->SetPlayerLfgMode(leader.guid, lfg::LfgCompositionMode::BOT_FILL);
        sCoAContentScaling->SetPlayerLfgChallenge(leader.guid, 0);

        // 17a: Even when GroupScaling.Enable = 0, LFG BotFill policy still works as long as CoAContentScaling.Enable = 1
        sCoAContentScaling->SetGroupScalingEnabled(false);
        lfg::LfgQueuePolicy policyDisabled{};
        sCoAContentScaling->OnResolveLfgQueuePolicy(leader.guid, policyDisabled);
        assert(policyDisabled.compositionMode == lfg::LfgCompositionMode::BOT_FILL);
        assert(policyDisabled.targetPlayers == 5);

        // 17b: With GroupScaling.Enable = 1 and Adaptive challengeSize = 0,
        // BOT_FILL does NOT artificially lock difficulty to 5 when fewer participants are present!
        sCoAContentScaling->SetGroupScalingEnabled(true);
        lfg::LfgProposal proposal;
        proposal.dungeonId = 30;
        sCoAContentScaling->OnResolveLfgQueuePolicy(leader.guid, proposal.policy);
        proposal.players[leader.guid] = {};
        sCoAContentScaling->OnLfgProposalMadeGroup(proposal, &group);

        InstanceMap deadmines;
        deadmines.mapId = 36;
        deadmines.instanceId = 117;
        deadmines.players = { &leader }; // Only human is in map initially (or solo walk-in)
        sCoAContentScaling->OnInstanceMapCreated(&deadmines, &leader);

        Creature boss = MakeBoss(&rhahkZorTpl, &deadmines, 8001);
        sCoAContentScaling->ApplyCreatureScaling(&rhahkZorTpl, &boss);
        // Solo in map -> 1-player HP (5,782), NOT artificially locked to 5!
        assert(boss.GetMaxHealth() == 5782);

        // Incomplete bot recruitment / partial teleport: only 2 bots arrive (3 total participants)
        Player b1{ObjectGuid(2), 19}, b2{ObjectGuid(3), 19}, b3{ObjectGuid(4), 49}, b4{ObjectGuid(5), 49};
        for (Player* b : { &b1, &b2 })
        {
            deadmines.players.push_back(b);
            sCoAContentScaling->RefreshDungeonCreatures(&deadmines);
        }
        assert(boss.GetMaxHealth() == ExpectedRhahkZorHp(3));

        // Remaining 2 bots arrive (5 total participants) -> scales to 5-player HP (26,671) before pull
        for (Player* b : { &b3, &b4 })
        {
            deadmines.players.push_back(b);
            sCoAContentScaling->RefreshDungeonCreatures(&deadmines);
        }
        assert(boss.GetMaxHealth() == 26671);

        // 1 bot disconnects before pull (inWorld = false in RemovePlayerFromMap) -> scales to 4-player HP
        b4.inWorld = false;
        sCoAContentScaling->RefreshDungeonCreatures(&deadmines);
        assert(boss.GetMaxHealth() == ExpectedRhahkZorHp(4));

        // Bot 4 reconnects before pull -> back to 5-player HP (26,671)
        b4.inWorld = true;
        sCoAContentScaling->RefreshDungeonCreatures(&deadmines);
        assert(boss.GetMaxHealth() == 26671);

        // Opening attack deals 671 damage before OnUnitEnterCombat fires -> preserved because maxHP did not change!
        boss.inCombat = true;
        boss.SetHealth(26000);
        sInstanceScalingMgr->BeginEncounter(
            &deadmines,
            EncounterLifecycleSource::CREATURE_FALLBACK,
            EncounterKey{EncounterKeyType::CREATURE_ENTRY, boss.GetEntry()},
            &boss);
        assert(boss.GetMaxHealth() == 26671);
        assert(boss.GetHealth() == 26000);
    }

    std::cout << "All 17 encounter & boss scaling regression scenarios passed!" << std::endl;
    return 0;
}
''', encoding='utf-8')

        exe = out / ('test_scenarios.exe' if os.name == 'nt' else 'test_scenarios')
        files = [
            'CombatBudgetProfile.cpp',
            'InstanceScaleContext.cpp',
            'InstanceProfile.cpp',
            'ContentPackRegistry.cpp',
            'ProgressionLayout.cpp',
            'GeneratedItemProfiles.cpp',
            'CoAContentScalingStub.cpp',
            'test_scenarios.cpp',
        ]
        flags = (
            ['/nologo', '/std:c++20', '/EHsc', '/utf-8', *files, '/Fe' + str(exe)]
            if Path(compiler).stem.lower() == 'cl'
            else ['-std=c++20', *files, '-o', str(exe)]
        )
        subprocess.run([compiler, *flags], cwd=out, check=True)
        subprocess.run([str(exe)], cwd=out, check=True)


if __name__ == '__main__':
    main()
