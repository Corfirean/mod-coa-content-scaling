/*
 * CoA Universal Content Scaling
 * CombatBudgetProfile: Authoritative combat budget calculator reconciling authored expansion
 * multipliers with target CoA effective base stats and tier modifiers.
 */

#ifndef COA_COMBAT_BUDGET_PROFILE_H
#define COA_COMBAT_BUDGET_PROFILE_H

#include "ContentTier.h"
#include "CreatureScaleContext.h"
#include "Define.h"

class Creature;
void RescaleLootDamageRequirement(Creature* creature, uint32 previousMaxHealth);
struct CreatureTemplate;

struct CalculatedCombatBudget
{
    uint32 health{100};
    uint32 mana{0};
    float minDamage{10.0f};
    float maxDamage{15.0f};
    float armor{100.0f};
    uint32 attackPower{50};

    // How `health` was built, for CoAContentScaling.Debug and for tests:
    // health = ceil(authoredHealth * tierHealthMultiplier * group scale * difficultyHealthMultiplier) * externalHealthMultiplier
    float authoredHealth{0.0f};              // class/level base health * the template's ModHealth
    float tierHealthMultiplier{1.0f};
    float difficultyHealthMultiplier{1.0f};  // CoAContentScaling.Difficulty.Health.* for this place and role
    float externalHealthMultiplier{1.0f};    // CoA.CreatureScaling (open world only when this module owns the map) * rank rate
};

class CombatBudgetProfile
{
public:
    static CombatBudgetProfile* Instance();

    // Resolves tier modifier for health, damage, armor
    static float GetTierHealthMultiplier(ContentTier tier);
    static float GetTierDamageMultiplier(ContentTier tier);
    static float GetTierArmorMultiplier(ContentTier tier);

    // Calculates complete combat budget for a creature context
    CalculatedCombatBudget CalculateBudget(CreatureTemplate const* cinfo,
                                          CreatureScaleContext const& context) const;

    // Authoritative boss identity of a template: a dungeon encounter boss (CREATURE_FLAG_EXTRA_DUNGEON_BOSS) or a
    // WORLDBOSS rank. Ordinary elite trash is not a boss.
    static bool IsBossTemplate(CreatureTemplate const* cinfo);

    // Builds scale context for a creature
    CreatureScaleContext BuildContext(CreatureTemplate const* cinfo, Creature const* creature,
                                     uint8 effectiveLevel) const;

private:
    CombatBudgetProfile() = default;
};

#define sCombatBudgetProfile CombatBudgetProfile::Instance()

#endif // COA_COMBAT_BUDGET_PROFILE_H
