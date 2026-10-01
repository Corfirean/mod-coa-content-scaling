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

struct ScaledItemBudget
{
    uint32 effectiveRequiredLevel{1};
    uint32 effectiveItemLevel{1};
    float statMultiplier{1.0f};
    float ratingMultiplier{1.0f};
    float armorMultiplier{1.0f};
    float weaponDpsMultiplier{1.0f};
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
