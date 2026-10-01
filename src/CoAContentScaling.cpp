/*
 * CoA Universal Content Scaling
 * CoAContentScaling: Implementation of master controller, scripts and hooks.
 */

#include "CoAContentScaling.h"
#include "AdaptiveEncounterAPI.h"
#include "CombatBudgetProfile.h"
#include "Config.h"
#include "ContentPackRegistry.h"
#include "Creature.h"
#include "CreatureData.h"
#include "DBCEnums.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "Group.h"
#include "InstanceScaleContext.h"
#include "ItemBudgetScaler.h"
#include "LFGMgr.h"
#include "LocalLevelScaling.h"
#include "Log.h"
#include "LootMgr.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QueryResult.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "SoloAssistPolicy.h"
#include "SpellAuraDefines.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "World.h"
#include <algorithm>
#include <cmath>

void AddCoAContentScalingCommands();

CoAContentScaling* CoAContentScaling::Instance()
{
    static CoAContentScaling instance;
    return &instance;
}

void CoAContentScaling::LoadConfig()
{
    _enabled = sConfigMgr->GetOption<bool>("CoAContentScaling.Enable", true);
    _groupScalingEnabled = sConfigMgr->GetOption<bool>("CoAContentScaling.GroupScaling.Enable", true);
    _lockOnEncounterStart = sConfigMgr->GetOption<bool>("CoAContentScaling.GroupScaling.LockOnEncounterStart", true);
    _adaptiveMechanicsEnabled = sConfigMgr->GetOption<bool>("CoAContentScaling.AdaptiveMechanics.Enable", true);
    _scaleLootCount = sConfigMgr->GetOption<bool>("CoAContentScaling.Rewards.ScaleLootCount", true);
    _allowSoloRaids = sConfigMgr->GetOption<bool>("CoAContentScaling.GroupScaling.AllowSoloRaids", true);
    _debug = sConfigMgr->GetOption<bool>("CoAContentScaling.Debug", false);

    _progressionMode = sConfigMgr->GetOption<std::string>("CoAContentScaling.ProgressionMode", "Auto");
    _customClassicEnd = static_cast<uint8>(sConfigMgr->GetOption<uint32>("CoAContentScaling.Progression.CustomClassicEnd", 45));
    _customTbcEnd = static_cast<uint8>(sConfigMgr->GetOption<uint32>("CoAContentScaling.Progression.CustomTbcEnd", 58));

    std::string const soloMode = sConfigMgr->GetOption<std::string>("CoAContentScaling.SoloAssist.Mode", "Light");
    if (soloMode == "Full")
        sSoloAssistPolicy->SetMode(SoloAssistMode::FULL);
    else if (soloMode == "None")
        sSoloAssistPolicy->SetMode(SoloAssistMode::NONE);
    else
        sSoloAssistPolicy->SetMode(SoloAssistMode::LIGHT);
}

void CoAContentScaling::FinalizeAndInitialize()
{
    if (sContentPackRegistry->IsFinalized())
        return;

    // 1. Finalize content pack registrations
    sContentPackRegistry->Finalize();

    // 2. Compute immutable progression layout
    InitializeLayout();

    // 3. Load calibrated boss flex profiles
    sInstanceScalingMgr->LoadCalibratedBossFlex();

    // 4. Scale item templates if enabled
    if (sConfigMgr->GetOption<bool>("CoAContentScaling.ScaleItems", true))
    {
        sItemBudgetScaler->ScaleAllItems(_layout);
    }

    // 5. Ensure DB table for character LFG settings exists
    CharacterDatabase.Execute(
        "CREATE TABLE IF NOT EXISTS `character_coa_lfg_settings` ("
        "`guid` INT UNSIGNED NOT NULL,"
        "`composition_mode` TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "`challenge_size` TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "PRIMARY KEY (`guid`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"
    );

    LOG_INFO("server.loading", "CoAContentScaling: Finalized lifecycle and built immutable ProgressionLayout (Cap {})",
             _layout.maxLevel);
}

void CoAContentScaling::InitializeLayout()
{
    uint8 const maxPlayerLevel = static_cast<uint8>(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
    bool const tbcActive = _tbcEnabled && sContentPackRegistry->HasPack(ContentEra::TBC);
    bool const wotlkActive = _wotlkEnabled && sContentPackRegistry->HasPack(ContentEra::WotLK);

    if (_progressionMode == "Custom")
    {
        _layout = ProgressionLayout::Create(
            maxPlayerLevel, tbcActive, wotlkActive, _customClassicEnd, _customTbcEnd);
    }
    else
    {
        _layout = ProgressionLayout::Create(maxPlayerLevel, tbcActive, wotlkActive);
    }

    std::string validationError;
    if (!_layout.Validate(validationError))
    {
        LOG_FATAL("server.loading", "CoAContentScaling: Progression layout invalid: {}", validationError);
        _enabled = false;
        return;
    }

    LOG_INFO("server.loading", "UniversalContentScaling: Active layout [Classic {}-{} | TBC: {} | WotLK: {}] Cap: {}",
             _layout.classic.minLevel, _layout.classic.maxLevel,
             _layout.tbc ? std::to_string(_layout.tbc->minLevel) + "-" + std::to_string(_layout.tbc->maxLevel) : "None",
             _layout.wotlk ? std::to_string(_layout.wotlk->minLevel) + "-" + std::to_string(_layout.wotlk->maxLevel) : "None",
             _layout.maxLevel);
}

uint8 CoAContentScaling::GetEffectiveCreatureLevel(CreatureTemplate const* cinfo, Creature const* creature, uint8 authoredLevel) const
{
    if (!_enabled || !cinfo)
        return authoredLevel;

    Map const* map = creature ? creature->GetMap() : nullptr;
    uint32 const mapId = map ? map->GetId() : 0;
    uint32 const areaId = creature ? creature->GetAreaId() : 0;

    ContentEra const era = sContentPackRegistry->ResolveEraForCreature(
        cinfo->Entry, mapId, areaId, cinfo->expansion, authoredLevel);

    return _layout.MapAuthoredToEffective(era, authoredLevel);
}

int32 CoAContentScaling::GetEffectiveQuestLevel(Quest const* quest) const
{
    if (!_enabled || !quest)
        return quest ? quest->GetQuestLevel() : 0;

    int32 const authoredLevel = quest->GetQuestLevel();
    if (authoredLevel <= 0)
        return authoredLevel;

    ContentEra const era = sContentPackRegistry->ResolveEraForQuest(
        quest->GetQuestId(), quest->GetZoneOrSort(), 0, static_cast<uint8>(authoredLevel));

    return static_cast<int32>(_layout.MapAuthoredToEffective(era, static_cast<uint8>(authoredLevel)));
}

uint32 CoAContentScaling::GetEffectiveQuestMinLevel(Quest const* quest) const
{
    if (!_enabled || !quest)
        return quest ? quest->GetMinLevel() : 0;

    uint32 const authoredMin = quest->GetMinLevel();
    if (authoredMin <= 1)
        return authoredMin;

    ContentEra const era = sContentPackRegistry->ResolveEraForQuest(
        quest->GetQuestId(), quest->GetZoneOrSort(), 0, static_cast<uint8>(authoredMin));

    return static_cast<uint32>(_layout.MapAuthoredToEffective(era, static_cast<uint8>(authoredMin)));
}

void CoAContentScaling::ApplyCreatureScaling(CreatureTemplate const* cinfo, Creature* creature)
{
    if (!_enabled || !cinfo || !creature)
        return;

    Map* map = creature->GetMap();
    CreatureScaleContext ctx = sCombatBudgetProfile->BuildContext(cinfo, creature, creature->GetLevel());

    uint32 calibratedHp = 0;
    if (_groupScalingEnabled && map && map->IsDungeon())
    {
        InstanceScaleContext instCtx = sInstanceScalingMgr->GetOrCreateContext(map);
        ctx.groupHealthScale = instCtx.healthScale;
        ctx.groupDamageScale = instCtx.damageScale;

        // Check calibrated coa_boss_flex profile
        if (map->IsRaid() && sInstanceScalingMgr->HasCalibratedBossHp(cinfo->Entry, map->GetSpawnMode()))
        {
            calibratedHp = sInstanceScalingMgr->GetCalibratedBossHp(
                cinfo->Entry, map->GetSpawnMode(), instCtx.effectivePlayers);
        }
    }

    CalculatedCombatBudget budget = sCombatBudgetProfile->CalculateBudget(cinfo, ctx);
    if (calibratedHp > 0)
        budget.health = calibratedHp;

    float const pct = creature->GetMaxHealth() ? creature->GetHealthPct() : 100.0f;
    creature->SetCreateHealth(budget.health);
    creature->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(budget.health));
    creature->UpdateMaxHealth();
    creature->SetHealth(std::max<uint32>(1, static_cast<uint32>(std::round(float(creature->GetMaxHealth()) * pct / 100.0f))));

    if (budget.mana > 0)
    {
        creature->SetCreateMana(budget.mana);
        creature->SetMaxPower(POWER_MANA, budget.mana);
        creature->SetPower(POWER_MANA, budget.mana);
        creature->SetStatFlatModifier(UNIT_MOD_MANA, BASE_VALUE, float(budget.mana));
    }

    creature->SetStatFlatModifier(UNIT_MOD_ARMOR, BASE_VALUE, budget.armor);
    creature->UpdateArmor();

    creature->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, float(budget.attackPower));
    creature->UpdateAttackPowerAndDamage();

    creature->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, budget.minDamage);
    creature->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, budget.maxDamage);
    creature->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, budget.minDamage);
    creature->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, budget.maxDamage);
    creature->SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, budget.minDamage);
    creature->SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, budget.maxDamage);

    creature->UpdateDamagePhysical(BASE_ATTACK);
    creature->UpdateDamagePhysical(OFF_ATTACK);
    creature->UpdateDamagePhysical(RANGED_ATTACK);
}

void CoAContentScaling::RecalculateEncounterCombatStats(Creature* boss, InstanceScaleContext const& snapshot)
{
    if (!_enabled || !boss)
        return;

    CreatureTemplate const* cinfo = boss->GetCreatureTemplate();
    if (!cinfo)
        return;

    Map* map = boss->GetMap();
    if (!map || !map->IsDungeon())
        return;

    CreatureScaleContext ctx = sCombatBudgetProfile->BuildContext(cinfo, boss, boss->GetLevel());
    ctx.groupHealthScale = snapshot.healthScale;
    ctx.groupDamageScale = snapshot.damageScale;

    uint32 calibratedHp = 0;
    if (map->IsRaid() && sInstanceScalingMgr->HasCalibratedBossHp(cinfo->Entry, map->GetSpawnMode()))
    {
        calibratedHp = sInstanceScalingMgr->GetCalibratedBossHp(
            cinfo->Entry, map->GetSpawnMode(), snapshot.effectivePlayers);
    }

    CalculatedCombatBudget budget = sCombatBudgetProfile->CalculateBudget(cinfo, ctx);
    if (calibratedHp > 0)
        budget.health = calibratedHp;

    float const pct = boss->GetMaxHealth() ? boss->GetHealthPct() : 100.0f;
    boss->SetCreateHealth(budget.health);
    boss->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(budget.health));
    boss->UpdateMaxHealth();
    boss->SetHealth(std::max<uint32>(1, static_cast<uint32>(std::round(float(boss->GetMaxHealth()) * pct / 100.0f))));

    if (budget.mana > 0)
    {
        boss->SetCreateMana(budget.mana);
        boss->SetMaxPower(POWER_MANA, budget.mana);
        boss->SetPower(POWER_MANA, budget.mana);
        boss->SetStatFlatModifier(UNIT_MOD_MANA, BASE_VALUE, float(budget.mana));
    }

    boss->SetStatFlatModifier(UNIT_MOD_ARMOR, BASE_VALUE, budget.armor);
    boss->UpdateArmor();

    boss->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, float(budget.attackPower));
    boss->UpdateAttackPowerAndDamage();

    boss->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, budget.minDamage);
    boss->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, budget.maxDamage);
    boss->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, budget.minDamage);
    boss->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, budget.maxDamage);
    boss->SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, budget.minDamage);
    boss->SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, budget.maxDamage);

    boss->UpdateDamagePhysical(BASE_ATTACK);
    boss->UpdateDamagePhysical(OFF_ATTACK);
    boss->UpdateDamagePhysical(RANGED_ATTACK);
}

bool CoAContentScaling::CanPlayerEnterMap(Player const* player, uint32 mapId) const
{
    if (!player || player->IsGameMaster())
        return true;

    ContentEra const era = sContentPackRegistry->ResolveEraForMap(mapId);
    if (!_layout.IsEraEnabled(era))
        return false;

    return true;
}

void CoAContentScaling::SetPlayerLfgMode(ObjectGuid guid, lfg::LfgCompositionMode mode)
{
    std::lock_guard<std::mutex> lock(_lfgSettingsLock);
    _playerLfgSettings[guid].compositionMode = mode;
}

lfg::LfgCompositionMode CoAContentScaling::GetPlayerLfgMode(ObjectGuid guid) const
{
    std::lock_guard<std::mutex> lock(_lfgSettingsLock);
    auto it = _playerLfgSettings.find(guid);
    return (it != _playerLfgSettings.end()) ? it->second.compositionMode : lfg::LfgCompositionMode::MATCHMAKING;
}

void CoAContentScaling::SetPlayerLfgChallenge(ObjectGuid guid, uint32 challengeSize)
{
    std::lock_guard<std::mutex> lock(_lfgSettingsLock);
    _playerLfgSettings[guid].challengeSize = challengeSize;
}

uint32 CoAContentScaling::GetPlayerLfgChallenge(ObjectGuid guid) const
{
    std::lock_guard<std::mutex> lock(_lfgSettingsLock);
    auto it = _playerLfgSettings.find(guid);
    return (it != _playerLfgSettings.end()) ? it->second.challengeSize : 0;
}

void CoAContentScaling::LoadPlayerLfgSettings(Player* player)
{
    if (!player)
        return;

    ObjectGuid const guid = player->GetGUID();
    QueryResult result = CharacterDatabase.Query(
        "SELECT composition_mode, challenge_size FROM character_coa_lfg_settings WHERE guid = {}",
        guid.GetCounter());

    std::lock_guard<std::mutex> lock(_lfgSettingsLock);
    if (result)
    {
        Field* fields = result->Fetch();
        _playerLfgSettings[guid].compositionMode = static_cast<lfg::LfgCompositionMode>(fields[0].Get<uint8>());
        _playerLfgSettings[guid].challengeSize = fields[1].Get<uint32>();
    }
    else
    {
        _playerLfgSettings[guid] = PlayerLfgSettings{};
    }
}

void CoAContentScaling::SavePlayerLfgSettings(Player* player)
{
    if (!player)
        return;

    ObjectGuid const guid = player->GetGUID();
    PlayerLfgSettings settings;
    {
        std::lock_guard<std::mutex> lock(_lfgSettingsLock);
        auto it = _playerLfgSettings.find(guid);
        if (it != _playerLfgSettings.end())
            settings = it->second;
    }

    CharacterDatabase.Execute(
        "REPLACE INTO character_coa_lfg_settings (guid, composition_mode, challenge_size) VALUES ({}, {}, {})",
        guid.GetCounter(), static_cast<uint8>(settings.compositionMode), settings.challengeSize);
}

void CoAContentScaling::OnResolveLfgQueuePolicy(ObjectGuid const& guid, lfg::LfgQueuePolicy& policy)
{
    PlayerLfgSettings settings;
    {
        std::lock_guard<std::mutex> lock(_lfgSettingsLock);
        auto it = _playerLfgSettings.find(guid);
        if (it != _playerLfgSettings.end())
            settings = it->second;
    }

    policy.compositionMode = settings.compositionMode;
    policy.challengeSize = settings.challengeSize;

    switch (settings.compositionMode)
    {
        case lfg::LfgCompositionMode::MATCHMAKING:
            policy.bypassMatchmaking = false;
            policy.requireStandardRoles = true;
            policy.minPlayers = 5;
            policy.targetPlayers = 5;
            break;

        case lfg::LfgCompositionMode::BOT_FILL:
            policy.bypassMatchmaking = false;
            policy.requireStandardRoles = true;
            policy.minPlayers = 5;
            policy.targetPlayers = 5;
            break;

        case lfg::LfgCompositionMode::CURRENT_PARTY:
            policy.bypassMatchmaking = true;
            policy.requireStandardRoles = false;
            policy.minPlayers = 1;
            Player* player = ObjectAccessor::FindPlayer(guid);
            Group* grp = player ? player->GetGroup() : nullptr;
            policy.targetPlayers = (settings.challengeSize > 0) ? settings.challengeSize : (grp ? grp->GetMembersCount() : 1);
            break;
    }
}

void CoAContentScaling::OnLfgProposalMadeGroup(lfg::LfgProposal const& proposal, Group* group)
{
    if (!group)
        return;

    lfg::LFGDungeonData const* dungeon = sLFGMgr->GetLFGDungeon(proposal.dungeonId);
    if (!dungeon)
        return;

    if (proposal.policy.challengeSize > 0)
    {
        sInstanceScalingMgr->SetChallengeSize(dungeon->map, group->GetGUID().GetCounter(), proposal.policy.challengeSize);
    }
}

// =============================================================================
// Script Hooks Integration
// =============================================================================

namespace
{
    class coa_content_scaling_world : public WorldScript
    {
    public:
        coa_content_scaling_world() : WorldScript("coa_content_scaling_world") { }

        void OnAfterConfigLoad(bool reload) override
        {
            if (reload && sContentPackRegistry->IsFinalized())
            {
                LOG_WARN("module.coa_content_scaling",
                         "UniversalContentScaling: Progression layout, expansion packs and MaxPlayerLevel cannot be changed at runtime! Server restart required.");
                return;
            }

            sCoAContentScaling->LoadConfig();
        }

        void OnLoadCustomDatabaseTable() override
        {
            sCoAContentScaling->FinalizeAndInitialize();

            bool const enabled = sCoAContentScaling->IsEnabled();
            LocalLevelScaling::ContentScalingActive.store(enabled, std::memory_order_relaxed);

            if (enabled)
            {
                LocalLevelScaling::QuestBaseLevelOwner.store([](Quest const* quest) -> int32
                {
                    return sCoAContentScaling->GetEffectiveQuestLevel(quest);
                }, std::memory_order_relaxed);

                LocalLevelScaling::CreatureBaseLevelOwner.store([](CreatureTemplate const* cinfo, Creature const* creature) -> uint8
                {
                    return sCoAContentScaling->GetEffectiveCreatureLevel(cinfo, creature, cinfo ? cinfo->maxlevel : 1);
                }, std::memory_order_relaxed);
            }
            else
            {
                LocalLevelScaling::QuestBaseLevelOwner.store(nullptr, std::memory_order_relaxed);
                LocalLevelScaling::CreatureBaseLevelOwner.store(nullptr, std::memory_order_relaxed);
            }
        }
    };

    class coa_content_scaling_global : public GlobalScript
    {
    public:
        coa_content_scaling_global() : GlobalScript("coa_content_scaling_global") { }

        void OnResolveLfgQueuePolicy(ObjectGuid const& guid, lfg::LfgQueuePolicy& policy) override
        {
            sCoAContentScaling->OnResolveLfgQueuePolicy(guid, policy);
        }

        void OnLfgProposalMadeGroup(lfg::LfgProposal const& proposal, Group* group) override
        {
            sCoAContentScaling->OnLfgProposalMadeGroup(proposal, group);
        }

        void OnBeforeSetBossState(uint32 id, EncounterState newState, EncounterState /*oldState*/, Map* instance) override
        {
            if (!sCoAContentScaling->IsEnabled() || !instance || !instance->IsDungeon())
                return;

            if (newState == IN_PROGRESS)
            {
                sInstanceScalingMgr->OnEncounterStart(instance, nullptr, id);
            }
            else if (newState == DONE || newState == FAIL || newState == NOT_STARTED)
            {
                sInstanceScalingMgr->OnEncounterEnd(instance, id);
            }
        }
    };

    class coa_content_scaling_creature : public AllCreatureScript
    {
    public:
        coa_content_scaling_creature() : AllCreatureScript("coa_content_scaling_creature") { }

        void OnBeforeCreatureSelectLevel(CreatureTemplate const* cinfo, Creature* creature, uint8& level) override
        {
            if (!sCoAContentScaling->IsEnabled())
                return;

            level = sCoAContentScaling->GetEffectiveCreatureLevel(cinfo, creature, level);
        }

        void OnCreatureSelectLevel(CreatureTemplate const* cinfo, Creature* creature) override
        {
            if (!sCoAContentScaling->IsEnabled())
                return;

            sCoAContentScaling->ApplyCreatureScaling(cinfo, creature);
        }
    };

    class coa_content_scaling_unit : public UnitScript
    {
    public:
        coa_content_scaling_unit() : UnitScript("coa_content_scaling_unit") { }

        void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
        {
            if (!sCoAContentScaling->IsEnabled() || !sCoAContentScaling->IsGroupScalingEnabled() || damage == 0)
                return;

            if (!attacker || !attacker->IsCreature() || !attacker->GetMap() || !attacker->GetMap()->IsDungeon())
                return;

            InstanceScaleContext const ctx = sInstanceScalingMgr->GetOrCreateContext(attacker->GetMap());
            bool const isSolo = (ctx.effectivePlayers <= 1.0f);
            float const soloMitigation = sSoloAssistPolicy->GetDamageMitigationMultiplier(isSolo, true);

            damage = static_cast<uint32>(std::ceil(float(damage) * ctx.damageScale * soloMitigation));
        }

        void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
        {
            if (!sCoAContentScaling->IsEnabled() || !sCoAContentScaling->IsGroupScalingEnabled() || damage <= 0)
                return;

            if (!attacker || !attacker->IsCreature() || !attacker->GetMap() || !attacker->GetMap()->IsDungeon())
                return;

            if (spellInfo && spellInfo->HasAttribute(SPELL_ATTR0_CU_AURA_CC))
                return;

            InstanceScaleContext const ctx = sInstanceScalingMgr->GetOrCreateContext(attacker->GetMap());
            bool const isSolo = (ctx.effectivePlayers <= 1.0f);
            float const soloMitigation = sSoloAssistPolicy->GetDamageMitigationMultiplier(isSolo, true);

            damage = static_cast<int32>(std::ceil(float(damage) * ctx.damageScale * soloMitigation));
        }

        void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* /*spellInfo*/) override
        {
            if (!sCoAContentScaling->IsEnabled() || !sCoAContentScaling->IsGroupScalingEnabled() || damage == 0)
                return;

            if (!attacker || !attacker->IsCreature() || !attacker->GetMap() || !attacker->GetMap()->IsDungeon())
                return;

            InstanceScaleContext const ctx = sInstanceScalingMgr->GetOrCreateContext(attacker->GetMap());
            damage = static_cast<uint32>(std::ceil(float(damage) * ctx.damageScale));
        }

        void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal, SpellInfo const* /*spellInfo*/) override
        {
            if (!sCoAContentScaling->IsEnabled() || !sCoAContentScaling->IsGroupScalingEnabled() || heal == 0)
                return;

            if (!target || !target->IsCreature() || !target->GetMap() || !target->GetMap()->IsDungeon())
                return;

            InstanceScaleContext const ctx = sInstanceScalingMgr->GetOrCreateContext(target->GetMap());
            heal = static_cast<uint32>(std::ceil(float(heal) * ctx.healingScale));
        }

        void OnAfterAuraEffectCalculateAmount(AuraEffect const* effect, Unit* caster, int32& amount) override
        {
            if (!sCoAContentScaling->IsEnabled() || !sCoAContentScaling->IsGroupScalingEnabled() || amount <= 0)
                return;

            if (!caster || !caster->IsCreature() || !caster->GetMap() || !caster->GetMap()->IsDungeon())
                return;

            if (effect && (effect->GetAuraType() == SPELL_AURA_SCHOOL_ABSORB ||
                           effect->GetAuraType() == SPELL_AURA_MANA_SHIELD))
            {
                InstanceScaleContext const ctx = sInstanceScalingMgr->GetOrCreateContext(caster->GetMap());
                amount = static_cast<int32>(std::ceil(float(amount) * ctx.absorbScale));
            }
        }

        void OnUnitEnterCombat(Unit* unit, Unit* /*victim*/) override
        {
            if (!sCoAContentScaling->IsEnabled() || !unit)
                return;

            Creature* creature = unit->ToCreature();
            if (!creature || !creature->GetMap() || !creature->GetMap()->IsDungeon())
                return;

            CreatureTemplate const* cinfo = creature->GetCreatureTemplate();
            // Authoritative encounter start: only bosses lock the encounter snapshot! Trash never locks.
            if (cinfo && (cinfo->rank == CREATURE_ELITE_WORLDBOSS || (cinfo->flags_extra & CREATURE_FLAG_EXTRA_DUNGEON_BOSS)))
            {
                sInstanceScalingMgr->OnEncounterStart(creature->GetMap(), creature, creature->GetEntry());
            }
        }

        void OnUnitExitCombat(Unit* unit) override
        {
            if (!sCoAContentScaling->IsEnabled() || !unit)
                return;

            Creature* creature = unit->ToCreature();
            if (!creature || !creature->GetMap() || !creature->GetMap()->IsDungeon())
                return;

            CreatureTemplate const* cinfo = creature->GetCreatureTemplate();
            if (cinfo && (cinfo->rank == CREATURE_ELITE_WORLDBOSS || (cinfo->flags_extra & CREATURE_FLAG_EXTRA_DUNGEON_BOSS)))
            {
                sInstanceScalingMgr->OnEncounterEnd(creature->GetMap(), creature->GetEntry());
            }
        }

        void OnUnitDeath(Unit* unit, Unit* /*killer*/) override
        {
            if (!sCoAContentScaling->IsEnabled() || !unit)
                return;

            Creature* creature = unit->ToCreature();
            if (!creature || !creature->GetMap() || !creature->GetMap()->IsDungeon())
                return;

            CreatureTemplate const* cinfo = creature->GetCreatureTemplate();
            if (cinfo && (cinfo->rank == CREATURE_ELITE_WORLDBOSS || (cinfo->flags_extra & CREATURE_FLAG_EXTRA_DUNGEON_BOSS)))
            {
                sInstanceScalingMgr->OnEncounterEnd(creature->GetMap(), creature->GetEntry());
            }
        }
    };

    class coa_content_scaling_player : public PlayerScript
    {
    public:
        coa_content_scaling_player() : PlayerScript("coa_content_scaling_player") { }

        bool OnPlayerCanEnterMap(Player* player, MapEntry const* entry, InstanceTemplate const* /*instance*/,
                                 MapDifficulty const* /*mapDiff*/, bool /*loginCheck*/) override
        {
            if (!sCoAContentScaling->IsEnabled())
                return true;

            return sCoAContentScaling->CanPlayerEnterMap(player, entry->MapID);
        }

        void OnPlayerLogin(Player* player) override
        {
            sCoAContentScaling->LoadPlayerLfgSettings(player);
        }

        void OnPlayerLogout(Player* player) override
        {
            sCoAContentScaling->SavePlayerLfgSettings(player);
        }
    };

    class coa_content_scaling_misc : public MiscScript
    {
    public:
        coa_content_scaling_misc() : MiscScript("coa_content_scaling_misc") { }

        void OnAfterLootTemplateProcess(Loot* loot, LootTemplate const* /*tab*/, LootStore const& /*store*/,
                                       Player* lootOwner, bool /*personal*/, bool /*noEmptyError*/,
                                       uint16 /*lootMode*/) override
        {
            if (!sCoAContentScaling->IsEnabled() || !loot || !lootOwner)
                return;

            Map* map = lootOwner->GetMap();
            if (!map || !map->IsDungeon())
                return;

            InstanceScaleContext const ctx = sInstanceScalingMgr->GetOrCreateContext(map);
            if (ctx.effectivePlayers >= float(ctx.intendedPlayers))
                return; // Full group, standard loot

            float const ratio = ctx.effectivePlayers / float(ctx.intendedPlayers);

            // Never eliminate all loot: keep at least 1 meaningful item
            if (loot->items.size() > 1)
            {
                uint32 const keepCount = std::max<uint32>(1, static_cast<uint32>(std::round(float(loot->items.size()) * ratio)));
                if (keepCount < loot->items.size())
                    loot->items.resize(keepCount);
            }
        }
    };
}

void AddCoAContentScalingScripts()
{
    new coa_content_scaling_world();
    new coa_content_scaling_global();
    new coa_content_scaling_creature();
    new coa_content_scaling_unit();
    new coa_content_scaling_player();
    new coa_content_scaling_misc();
    AddCoAContentScalingCommands();
}
