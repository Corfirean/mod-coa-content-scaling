/*
 * CoA Universal Content Scaling
 * ItemBudgetScaler: Normalization of item stats, ratings, armor, weapon DPS and required levels.
 */

#ifndef COA_ITEM_BUDGET_SCALER_H
#define COA_ITEM_BUDGET_SCALER_H

#include "ContentEra.h"
#include "ContentTier.h"
#include "Define.h"
#include "ProgressionLayout.h"

struct ItemTemplate;

enum class ItemModCategory : uint8
{
    PrimaryStat     = 0,
    SecondaryRating = 1,
    SpellPower      = 2,
    AttackPower     = 3,
    BlockValue      = 4,
    Resistance      = 5,
    Other           = 6
};

[[nodiscard]] constexpr ItemModCategory GetItemModCategory(uint32 statType)
{
    switch (statType)
    {
        case 3:  // ITEM_MOD_AGILITY
        case 4:  // ITEM_MOD_STRENGTH
        case 5:  // ITEM_MOD_INTELLECT
        case 6:  // ITEM_MOD_SPIRIT
        case 7:  // ITEM_MOD_STAMINA
            return ItemModCategory::PrimaryStat;

        case 12: // ITEM_MOD_DEFENSE_SKILL_RATING
        case 13: // ITEM_MOD_DODGE_RATING
        case 14: // ITEM_MOD_PARRY_RATING
        case 15: // ITEM_MOD_BLOCK_RATING
        case 16: // ITEM_MOD_HIT_MELEE_RATING
        case 17: // ITEM_MOD_HIT_RANGED_RATING
        case 18: // ITEM_MOD_HIT_SPELL_RATING
        case 19: // ITEM_MOD_CRIT_MELEE_RATING
        case 20: // ITEM_MOD_CRIT_RANGED_RATING
        case 21: // ITEM_MOD_CRIT_SPELL_RATING
        case 28: // ITEM_MOD_HASTE_MELEE_RATING
        case 29: // ITEM_MOD_HASTE_RANGED_RATING
        case 30: // ITEM_MOD_HASTE_SPELL_RATING
        case 31: // ITEM_MOD_HIT_RATING
        case 32: // ITEM_MOD_CRIT_RATING
        case 35: // ITEM_MOD_RESILIENCE_RATING
        case 36: // ITEM_MOD_HASTE_RATING
        case 37: // ITEM_MOD_EXPERTISE_RATING
        case 44: // ITEM_MOD_ARMOR_PENETRATION_RATING
            return ItemModCategory::SecondaryRating;

        case 38: // ITEM_MOD_ATTACK_POWER
        case 39: // ITEM_MOD_RANGED_ATTACK_POWER
        case 40: // ITEM_MOD_FERAL_ATTACK_POWER
            return ItemModCategory::AttackPower;

        case 45: // ITEM_MOD_SPELL_POWER
        case 41: // ITEM_MOD_SPELL_HEALING_DONE
        case 42: // ITEM_MOD_SPELL_DAMAGE_DONE
        case 43: // ITEM_MOD_MANA_REGENERATION
            return ItemModCategory::SpellPower;

        case 48: // ITEM_MOD_BLOCK_VALUE
            return ItemModCategory::BlockValue;

        case 22: // ITEM_MOD_HIT_TAKEN_MELEE_RATING
        case 23: // ITEM_MOD_HIT_TAKEN_RANGED_RATING
        case 24: // ITEM_MOD_HIT_TAKEN_SPELL_RATING
        case 25: // ITEM_MOD_CRIT_TAKEN_MELEE_RATING
        case 26: // ITEM_MOD_CRIT_TAKEN_RANGED_RATING
        case 27: // ITEM_MOD_CRIT_TAKEN_SPELL_RATING
            return ItemModCategory::SecondaryRating;

        default:
            return ItemModCategory::Other;
    }
}

struct ScaledItemBudget
{
    uint32 effectiveRequiredLevel{1};
    uint32 effectiveItemLevel{1};
    float statMultiplier{1.0f};
    float ratingMultiplier{1.0f};
    float armorMultiplier{1.0f};
    float weaponDpsMultiplier{1.0f};
    float spellPowerMultiplier{1.0f};
};

class ItemBudgetScaler
{
public:
    static ItemBudgetScaler* Instance();

    // Reconcile and calculate scaled budget for an item
    ScaledItemBudget CalculateItemBudget(ItemTemplate const* proto, ProgressionLayout const& layout) const;

    // Apply scaled budget onto ItemTemplate in-memory (called post ObjectMgr load)
    void ScaleItemTemplate(ItemTemplate* proto, ProgressionLayout const& layout) const;

    // Process all loaded items in ObjectMgr
    void ScaleAllItems(ProgressionLayout const& layout);

private:
    ItemBudgetScaler() = default;
};

#define sItemBudgetScaler ItemBudgetScaler::Instance()

#endif // COA_ITEM_BUDGET_SCALER_H
