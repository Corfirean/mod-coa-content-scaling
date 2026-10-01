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
#include "InstanceScaleContext.h"
#include "ItemBudgetScaler.h"
#include "LocalLevelScaling.h"
#include "LootMgr.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "SoloAssistPolicy.h"
#include "SpellAuraDefines.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "World.h"
#include <algorithm>

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
    _allowSoloRaids = sConfigMgr->GetOption<bool>("CoAContentScaling.AllowSoloRaids", true);
    _debug = sConfigMgr->GetOption<bool>("CoAContentScaling.Debug", false);

    _progressionMode = sConfigMgr->GetOption<std::string>("CoAContentScaling.Progression.Mode", "Auto");
    _customClassicEnd = static_cast<uint8>(sConfigMgr->GetOption<uint32>("CoAContentScaling.Progression.ClassicEnd", 0));
    _customTbcEnd     = static_cast<uint8>(sConfigMgr->GetOption<uint32>("CoAContentScaling.Progression.TbcEnd", 0));

    uint32 soloAssistMode = sConfigMgr->GetOption<uint32>("CoAContentScaling.SoloAssist.Mode", 0);
    sSoloAssistPolicy->SetMode(static_cast<SoloAssistMode>(std::min(soloAssistMode, 2u)));

    LOG_INFO("server.loading", "UniversalContentScaling: Configuration loaded (Enabled: {})", _enabled);
}

void CoAContentScaling::InitializeLayout()
{
    uint32 const maxPlayerLevel = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
    uint8 const cap = static_cast<uint8>(std::clamp<uint32>(maxPlayerLevel, 1, 255));

    _layout = ProgressionLayout::Create(cap, _tbcEnabled, _wotlkEnabled, _customClassicEnd, _customTbcEnd);

    std::string err;
    if (!_layout.Validate(err))
    {
        LOG_ERROR("server.loading", "UniversalContentScaling: ProgressionLayout validation failed: {}. Falling back to Classic-only layout.", err);
        _layout = ProgressionLayout::Create(cap, false, false);
    }

    LOG_INFO("server.loading", "UniversalContentScaling: Active Progression Layout:\n{}", _layout.ToString());
}

uint8 CoAContentScaling::GetEffectiveCreatureLevel(CreatureTemplate const* cinfo, Creature const* creature,
                                                  uint8 authoredLevel) const
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

    // Apply instance group scaling if in dungeon/raid
    if (_groupScalingEnabled && map && map->IsDungeon())
    {
        InstanceScaleContext instCtx = sInstanceScalingMgr->GetOrCreateContext(map);
        ctx.groupHealthScale = instCtx.healthScale;
        ctx.groupDamageScale = instCtx.damageScale;

        // Check calibrated coa_boss_flex profile
        if (map->IsRaid() && sInstanceScalingMgr->HasCalibratedBossHp(cinfo->Entry, map->GetSpawnMode()))
        {
            uint32 const calibratedHp = sInstanceScalingMgr->GetCalibratedBossHp(
                cinfo->Entry, map->GetSpawnMode(), instCtx.effectivePlayers);

            if (calibratedHp > 0)
            {
                float const pct = creature->GetMaxHealth() ? creature->GetHealthPct() : 100.0f;
                creature->SetCreateHealth(calibratedHp);
                creature->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(calibratedHp));
                creature->UpdateMaxHealth();
                creature->SetHealth(std::max<uint32>(1, static_cast<uint32>(creature->GetMaxHealth() * pct / 100.0f)));
                return;
            }
        }
    }

    CalculatedCombatBudget const budget = sCombatBudgetProfile->CalculateBudget(cinfo, ctx);

    float const pct = creature->GetMaxHealth() ? creature->GetHealthPct() : 100.0f;
    creature->SetCreateHealth(budget.health);
    creature->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(budget.health));
    creature->UpdateMaxHealth();
    creature->SetHealth(std::max<uint32>(1, static_cast<uint32>(creature->GetMaxHealth() * pct / 100.0f)));

    if (budget.mana > 0)
    {
        creature->SetCreateMana(budget.mana);
        creature->SetMaxPower(POWER_MANA, budget.mana);
        creature->SetPower(POWER_MANA, budget.mana);
        creature->SetStatFlatModifier(UNIT_MOD_MANA, BASE_VALUE, float(budget.mana));
    }

    creature->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, budget.minDamage);
    creature->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, budget.maxDamage);
    creature->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, budget.minDamage);
    creature->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, budget.maxDamage);
    creature->SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, budget.minDamage);
    creature->SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, budget.maxDamage);
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

// =============================================================================
// Script Hooks Integration
// =============================================================================

namespace
{
    class coa_content_scaling_world : public WorldScript
    {
    public:
        coa_content_scaling_world() : WorldScript("coa_content_scaling_world") { }

        void OnAfterConfigLoad(bool /*reload*/) override
        {
            sCoAContentScaling->LoadConfig();
            sCoAContentScaling->InitializeLayout();

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

        void OnLoadCustomDatabaseTable() override
        {
            sInstanceScalingMgr->LoadCalibratedBossFlex();
        }

        void OnStartup() override
        {
            if (sCoAContentScaling->IsEnabled())
                sItemBudgetScaler->ScaleAllItems(sCoAContentScaling->GetLayout());
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
        coa_content_scaling_unit() : UnitScript("coa_content_scaling_unit", true,
            {
                UNITHOOK_MODIFY_MELEE_DAMAGE,
                UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN,
                UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK,
                UNITHOOK_MODIFY_HEAL_RECEIVED,
                UNITHOOK_ON_AFTER_AURA_EFFECT_CALCULATE_AMOUNT,
                UNITHOOK_ON_UNIT_ENTER_COMBAT,
                UNITHOOK_ON_UNIT_EXIT_COMBAT,
                UNITHOOK_ON_UNIT_DEATH
            }) { }

        void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
        {
            if (!sCoAContentScaling->IsEnabled() || !sCoAContentScaling->IsGroupScalingEnabled())
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

            // Exclude percentage-of-health or instant death effects
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
            if (cinfo && (cinfo->rank == CREATURE_ELITE_WORLDBOSS || creature->GetMap()->IsRaid()))
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
            if (cinfo && (cinfo->rank == CREATURE_ELITE_WORLDBOSS || creature->GetMap()->IsRaid()))
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
            if (cinfo && (cinfo->rank == CREATURE_ELITE_WORLDBOSS || creature->GetMap()->IsRaid()))
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

            // Ratio of drop slots
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
    new coa_content_scaling_creature();
    new coa_content_scaling_unit();
    new coa_content_scaling_player();
    new coa_content_scaling_misc();
    AddCoAContentScalingCommands();
}
