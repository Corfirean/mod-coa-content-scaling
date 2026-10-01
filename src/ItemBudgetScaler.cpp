/*
 * CoA Universal Content Scaling
 * ItemBudgetScaler: Implementation of item stat, rating and level budget scaling.
 */

#include "ItemBudgetScaler.h"
#include "ContentPackRegistry.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "ObjectMgr.h"
#include <algorithm>
#include <cmath>

ItemBudgetScaler* ItemBudgetScaler::Instance()
{
    static ItemBudgetScaler instance;
    return &instance;
}

ScaledItemBudget ItemBudgetScaler::CalculateItemBudget(ItemTemplate const* proto,
                                                      ProgressionLayout const& layout) const
{
    ScaledItemBudget budget;
    if (!proto)
        return budget;

    ContentEra const era = sContentPackRegistry->ResolveEraForItem(
        proto->ItemId, proto->ItemLevel, proto->RequiredLevel);

    budget.effectiveRequiredLevel = proto->RequiredLevel;
    budget.effectiveItemLevel = proto->ItemLevel;

    // If item has no level requirement or is purely cosmetic/quest, do not alter required level
    if (proto->RequiredLevel > 0)
    {
        switch (era)
        {
            case ContentEra::Classic:
                budget.effectiveRequiredLevel = layout.MapAuthoredToEffective(
                    ContentEra::Classic, proto->RequiredLevel, 1, 60);
                break;
            case ContentEra::TBC:
                budget.effectiveRequiredLevel = layout.MapAuthoredToEffective(
                    ContentEra::TBC, proto->RequiredLevel, 58, 70);
                break;
            case ContentEra::WotLK:
                budget.effectiveRequiredLevel = layout.MapAuthoredToEffective(
                    ContentEra::WotLK, proto->RequiredLevel, 68, 80);
                break;
            default:
                budget.effectiveRequiredLevel = std::min<uint32>(proto->RequiredLevel, layout.maxLevel);
                break;
        }
    }

    // Clamp required level to realm cap
    budget.effectiveRequiredLevel = std::min<uint32>(budget.effectiveRequiredLevel, layout.maxLevel);

    // If realm cap is 80 and all eras active, stock budgets are largely valid
    if (layout.maxLevel == 80 && layout.tbcEnabled && layout.wotlkEnabled)
        return budget;

    // Normalizing item power budget for compressed realms (e.g. Cap 60)
    // Authored WotLK ilvl ranges: 150-284. On cap 60 realm, endgame items target ilvl ~60-85.
    uint32 targetMaxIlvl = layout.maxLevel + 25; // e.g. 85 for cap 60, 105 for cap 80
    if (proto->ItemLevel > targetMaxIlvl)
    {
        float const ilvlRatio = float(targetMaxIlvl) / float(proto->ItemLevel);
        budget.statMultiplier = std::clamp<float>(ilvlRatio, 0.30f, 1.0f);

        // Combat rating deflation: WotLK rating at lower level gives inflated percentages without conversion adjustment
        float const levelRatio = (proto->RequiredLevel > 0) ?
            (float(budget.effectiveRequiredLevel) / float(proto->RequiredLevel)) : 0.75f;
        budget.ratingMultiplier = std::clamp<float>(ilvlRatio * levelRatio, 0.20f, 1.0f);

        budget.armorMultiplier = budget.statMultiplier;
        budget.weaponDpsMultiplier = budget.statMultiplier;
        budget.effectiveItemLevel = static_cast<uint32>(std::round(float(proto->ItemLevel) * ilvlRatio));
    }

    return budget;
}

void ItemBudgetScaler::ScaleItemTemplate(ItemTemplate* proto, ProgressionLayout const& layout) const
{
    if (!proto)
        return;

    ScaledItemBudget const budget = CalculateItemBudget(proto, layout);

    proto->RequiredLevel = budget.effectiveRequiredLevel;
    proto->ItemLevel     = budget.effectiveItemLevel;

    if (budget.statMultiplier < 1.0f)
    {
        for (uint32 i = 0; i < proto->StatsCount; ++i)
        {
            if (proto->ItemStat[i].ItemStatValue != 0)
            {
                // Differentiate primary stats vs combat ratings
                uint32 const statType = proto->ItemStat[i].ItemStatType;
                bool const isRating = (statType >= ITEM_MOD_DEFENSE_SKILL_RATING &&
                                       statType <= ITEM_MOD_BLOCK_VALUE);

                float const mult = isRating ? budget.ratingMultiplier : budget.statMultiplier;
                proto->ItemStat[i].ItemStatValue = static_cast<int32>(
                    std::round(float(proto->ItemStat[i].ItemStatValue) * mult));
            }
        }

        // Scale weapon damage
        for (uint32 i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
        {
            if (proto->Damage[i].DamageMax > 0.0f)
            {
                proto->Damage[i].DamageMin *= budget.weaponDpsMultiplier;
                proto->Damage[i].DamageMax *= budget.weaponDpsMultiplier;
            }
        }

        // Scale armor
        if (proto->Armor > 0)
        {
            proto->Armor = static_cast<uint32>(
                std::round(float(proto->Armor) * budget.armorMultiplier));
        }
    }
}

void ItemBudgetScaler::ScaleAllItems(ProgressionLayout const& layout)
{
    ItemTemplateContainer const* itemTemplates = sObjectMgr->GetItemTemplateStore();
    if (!itemTemplates)
        return;

    uint32 scaledCount = 0;
    for (auto& [itemId, itemTemplate] : *itemTemplates)
    {
        ScaleItemTemplate(const_cast<ItemTemplate*>(&itemTemplate), layout);
        ++scaledCount;
    }

    LOG_INFO("server.loading", ">> UniversalContentScaling: Normalized budgets for {} items according to progression layout",
             scaledCount);
}
