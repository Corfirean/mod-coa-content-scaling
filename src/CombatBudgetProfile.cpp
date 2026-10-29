/*
 * CoA Universal Content Scaling
 * CombatBudgetProfile: Implementation of budget normalization and creature scaling.
 */

#include "CombatBudgetProfile.h"
#include "CoAContentScaling.h"
#include "ContentPackRegistry.h"
#include "CoACreatureScaling.h"
#include "CoACreatureScalingPolicy.h"
#include "Creature.h"
#include "CreatureData.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "World.h"
#include <algorithm>
#include <cmath>

CombatBudgetProfile* CombatBudgetProfile::Instance()
{
    static CombatBudgetProfile instance;
    return &instance;
}

float CombatBudgetProfile::GetTierHealthMultiplier(ContentTier tier)
{
    switch (tier)
    {
        case ContentTier::WORLD:          return 1.0f;
        // Normal dungeons take the authored budget (ModHealth) as it is; group scaling alone adapts it to the party.
        case ContentTier::DUNGEON_NORMAL: return 1.0f;
        case ContentTier::DUNGEON_HEROIC: return 2.20f;
        case ContentTier::RAID_ENTRY:     return 3.50f;
        case ContentTier::RAID_MID:       return 4.50f;
        case ContentTier::RAID_END:       return 6.00f;
        case ContentTier::RAID_PINNACLE:  return 8.00f;
        default:                          return 1.0f;
    }
}

float CombatBudgetProfile::GetTierDamageMultiplier(ContentTier tier)
{
    switch (tier)
    {
        case ContentTier::WORLD:          return 1.0f;
        case ContentTier::DUNGEON_NORMAL: return 1.15f;
        case ContentTier::DUNGEON_HEROIC: return 1.45f;
        case ContentTier::RAID_ENTRY:     return 1.80f;
        case ContentTier::RAID_MID:       return 2.10f;
        case ContentTier::RAID_END:       return 2.50f;
        case ContentTier::RAID_PINNACLE:  return 3.00f;
        default:                          return 1.0f;
    }
}

float CombatBudgetProfile::GetTierArmorMultiplier(ContentTier tier)
{
    switch (tier)
    {
        case ContentTier::WORLD:          return 1.0f;
        case ContentTier::DUNGEON_NORMAL: return 1.05f;
        case ContentTier::DUNGEON_HEROIC: return 1.15f;
        case ContentTier::RAID_ENTRY:     return 1.25f;
        case ContentTier::RAID_MID:       return 1.30f;
        case ContentTier::RAID_END:       return 1.35f;
        case ContentTier::RAID_PINNACLE:  return 1.40f;
        default:                          return 1.0f;
    }
}

bool CombatBudgetProfile::IsBossTemplate(CreatureTemplate const* cinfo)
{
    // CREATURE_FLAG_EXTRA_DUNGEON_BOSS is set at load from instance_encounters, so it names the real encounter bosses;
    // a rank of WORLDBOSS is the other authored boss identity. Rank ELITE alone is ordinary trash.
    return cinfo && (cinfo->rank == CREATURE_ELITE_WORLDBOSS || (cinfo->flags_extra & CREATURE_FLAG_EXTRA_DUNGEON_BOSS));
}

CreatureScaleContext CombatBudgetProfile::BuildContext(CreatureTemplate const* cinfo,
                                                      Creature const* creature,
                                                      uint8 effectiveLevel) const
{
    CreatureScaleContext context;
    if (!cinfo)
        return context;

    context.authoredLevel = cinfo->maxlevel;
    context.effectiveLevel = effectiveLevel;
    context.healthModifier = CreatureScaling::HealthMultiplier(creature);

    Map const* map = creature ? creature->GetMap() : nullptr;
    uint32 const mapId = map ? map->GetId() : 0;
    uint32 const areaId = creature ? creature->GetAreaId() : 0;

    // Resolve era
    context.era = sContentPackRegistry->ResolveEraForCreature(
        cinfo->Entry, mapId, areaId, cinfo->expansion, cinfo->maxlevel);

    if (map)
    {
        context.dungeon = map->IsDungeon();
        context.raid    = map->IsRaid();
        context.heroic  = map->IsHeroic();
    }

    context.boss = IsBossTemplate(cinfo);
    context.worldBoss = cinfo->rank == CREATURE_ELITE_WORLDBOSS && !context.dungeon && !context.raid;

    // Determine tier
    if (context.raid)
    {
        if (context.boss)
            context.tier = context.heroic ? ContentTier::RAID_END : ContentTier::RAID_MID;
        else
            context.tier = ContentTier::RAID_ENTRY;
    }
    else if (context.dungeon)
    {
        context.tier = context.heroic ? ContentTier::DUNGEON_HEROIC : ContentTier::DUNGEON_NORMAL;
    }
    else
    {
        context.tier = ContentTier::WORLD;
    }

    return context;
}

CalculatedCombatBudget CombatBudgetProfile::CalculateBudget(CreatureTemplate const* cinfo,
                                                           CreatureScaleContext const& context) const
{
    CalculatedCombatBudget budget;
    if (!cinfo)
        return budget;

    // Retrieve target base stats at the effective level
    CreatureBaseStats const* targetStats = sObjectMgr->GetCreatureBaseStats(context.effectiveLevel, cinfo->unit_class);
    if (!targetStats)
        return budget;

    // Base stats in creature_classlevelstats have columns for expansions:
    // We normalize to expansion 0 (CoA standard budget) to avoid WotLK column inflation at compressed levels:
    uint32 const baseHealth = targetStats->BaseHealth[0];
    float const baseDamage  = targetStats->BaseDamage[0];
    float const baseArmor   = targetStats->BaseArmor;
    uint32 const baseMana   = targetStats->BaseMana;

    float const tierHealthMod = GetTierHealthMultiplier(context.tier);
    bool const classicNormalDungeon =
        context.era == ContentEra::Classic && context.tier == ContentTier::DUNGEON_NORMAL;
    float const tierDmgMod    = classicNormalDungeon ? 1.0f : GetTierDamageMultiplier(context.tier);
    float const tierArmorMod  = classicNormalDungeon ? 1.0f : GetTierArmorMultiplier(context.tier);

    // Health: base * authored ModHealth * tier modifier * group scale * the server's own health setting for this kind of
    // place and role (CoAContentScaling.Difficulty.Health.*). ModHealth already carries what the creature is: ordinary
    // elites are authored around 3, encounter bosses around 5 to 20, world bosses in the hundreds, so there is no
    // further elite or world boss factor here. CoA.CreatureScaling only joins in through context.healthModifier, and
    // for dungeon and raid creatures this module owns that is neutral (see CoAContentScaling::OwnsCombatScaling).
    float const difficultyHealthMod =
        sCoAContentScaling->GetHealthMultiplier(context.dungeon, context.raid, context.boss || context.worldBoss);
    budget.authoredHealth = float(baseHealth) * cinfo->ModHealth;
    budget.tierHealthMultiplier = tierHealthMod;
    budget.difficultyHealthMultiplier = difficultyHealthMod;
    float const finalHealth = budget.authoredHealth * tierHealthMod * context.groupHealthScale * difficultyHealthMod;

    ServerConfigs healthRate = RATE_CREATURE_ELITE_ELITE_HP;
    switch (cinfo->rank)
    {
        case CREATURE_ELITE_NORMAL: healthRate = RATE_CREATURE_NORMAL_HP; break;
        case CREATURE_ELITE_ELITE: healthRate = RATE_CREATURE_ELITE_ELITE_HP; break;
        case CREATURE_ELITE_RAREELITE: healthRate = RATE_CREATURE_ELITE_RAREELITE_HP; break;
        case CREATURE_ELITE_WORLDBOSS: healthRate = RATE_CREATURE_ELITE_WORLDBOSS_HP; break;
        case CREATURE_ELITE_RARE: healthRate = RATE_CREATURE_ELITE_RARE_HP; break;
        default: break;
    }
    budget.externalHealthMultiplier = context.healthModifier * sWorld->getRate(healthRate);
    budget.health = CreatureScaling::Scaled(std::max<uint32>(1, static_cast<uint32>(std::ceil(finalHealth))),
        budget.externalHealthMultiplier);

    // Mana: base * authored multiplier
    if (baseMana > 0)
    {
        float const finalMana = float(baseMana) * cinfo->ModMana;
        budget.mana = static_cast<uint32>(std::ceil(finalMana));
    }
    else
    {
        budget.mana = 0;
    }

    // Armor: base * authored multiplier * tier modifier
    budget.armor = std::ceil(baseArmor * cinfo->ModArmor * tierArmorMod);

    // The core applies the authored DamageModifier and rank rate when deriving physical damage from this range.
    // Group scaling is applied at runtime in damage modification hooks.
    float const baseMinDmg = baseDamage * tierDmgMod;
    budget.minDamage = std::max(1.0f, baseMinDmg);
    budget.maxDamage = std::max(budget.minDamage, baseMinDmg * 1.5f);

    budget.attackPower = targetStats->AttackPower;

    return budget;
}
