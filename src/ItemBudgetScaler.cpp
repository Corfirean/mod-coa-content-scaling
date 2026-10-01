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

namespace
{
    struct PowerBandMapping
    {
        uint32 authMin;
        uint32 authMax;
        uint32 effMin;
        uint32 effMax;
    };

    uint32 MapThroughBand(uint32 authoredIlvl, PowerBandMapping const& band)
    {
        if (authoredIlvl <= band.authMin)
            return band.effMin;
        if (authoredIlvl >= band.authMax)
            return band.effMax;

        float const t = float(authoredIlvl - band.authMin) / float(std::max<uint32>(1, band.authMax - band.authMin));
        return band.effMin + static_cast<uint32>(std::round(t * float(band.effMax - band.effMin)));
    }
}

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

    // 1. Required level scaling
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

    budget.effectiveRequiredLevel = std::min<uint32>(budget.effectiveRequiredLevel, layout.maxLevel);

    // If realm cap is 80 and all eras active, stock budgets are valid
    if (layout.maxLevel == 80 && layout.tbcEnabled && layout.wotlkEnabled)
        return budget;

    // 2. Monotonic Power Bands for compressed realms
    // Avoid flattening higher raid tiers: maintain Naxx < Ulduar < ToC < ICC hierarchy
    uint32 newIlvl = proto->ItemLevel;

    if (era == ContentEra::Classic)
    {
        LevelRange const& cr = layout.classic;
        if (cr.maxLevel < 60)
        {
            // Classic leveling: [1, 60] -> [1, cr.maxLevel]
            // Classic raid endgame: [61, 92] -> [cr.maxLevel + 1, cr.maxLevel + 25]
            if (proto->ItemLevel <= 60)
            {
                newIlvl = MapThroughBand(proto->ItemLevel, { 1, 60, 1, cr.maxLevel });
            }
            else
            {
                newIlvl = MapThroughBand(proto->ItemLevel, { 61, 92, uint32(cr.maxLevel + 1), uint32(cr.maxLevel + 25) });
            }
        }
    }
    else if (era == ContentEra::TBC && layout.tbc.has_value())
    {
        LevelRange const& tr = *layout.tbc;
        if (proto->ItemLevel <= 115) // TBC Leveling
        {
            newIlvl = MapThroughBand(proto->ItemLevel, { 85, 115, tr.minLevel, tr.maxLevel });
        }
        else if (proto->ItemLevel <= 125) // T4
        {
            newIlvl = MapThroughBand(proto->ItemLevel, { 116, 125, uint32(tr.maxLevel + 1), uint32(tr.maxLevel + 8) });
        }
        else if (proto->ItemLevel <= 141) // T5
        {
            newIlvl = MapThroughBand(proto->ItemLevel, { 126, 141, uint32(tr.maxLevel + 9), uint32(tr.maxLevel + 16) });
        }
        else // T6 & Sunwell
        {
            newIlvl = MapThroughBand(proto->ItemLevel, { 142, 164, uint32(tr.maxLevel + 17), uint32(tr.maxLevel + 26) });
        }
    }
    else if (era == ContentEra::WotLK && layout.wotlk.has_value())
    {
        LevelRange const& wr = *layout.wotlk;
        if (proto->ItemLevel <= 187) // WotLK Leveling / Normals
        {
            newIlvl = MapThroughBand(proto->ItemLevel, { 138, 187, wr.minLevel, wr.maxLevel });
        }
        else if (proto->ItemLevel <= 226) // Tier 7 (Naxx/OS/EoE)
        {
            newIlvl = MapThroughBand(proto->ItemLevel, { 188, 226, uint32(wr.maxLevel + 1), uint32(wr.maxLevel + 9) });
        }
        else if (proto->ItemLevel <= 252) // Tier 8 (Ulduar)
        {
            newIlvl = MapThroughBand(proto->ItemLevel, { 227, 252, uint32(wr.maxLevel + 10), uint32(wr.maxLevel + 18) });
        }
        else if (proto->ItemLevel <= 258) // Tier 9 (Trial of the Crusader)
        {
            newIlvl = MapThroughBand(proto->ItemLevel, { 253, 258, uint32(wr.maxLevel + 19), uint32(wr.maxLevel + 26) });
        }
        else // Tier 10 & Ruby Sanctum (ICC/RS)
        {
            newIlvl = MapThroughBand(proto->ItemLevel, { 259, 284, uint32(wr.maxLevel + 27), uint32(wr.maxLevel + 36) });
        }
    }

    budget.effectiveItemLevel = newIlvl;

    if (proto->ItemLevel > 0 && budget.effectiveItemLevel < proto->ItemLevel)
    {
        float const ilvlRatio = float(budget.effectiveItemLevel) / float(proto->ItemLevel);
        budget.statMultiplier = std::clamp<float>(ilvlRatio, 0.20f, 1.0f);

        // Combat rating scaling: ratings provide inflated % at lower level without deflation
        float const levelRatio = (proto->RequiredLevel > 0) ?
            (float(budget.effectiveRequiredLevel) / float(proto->RequiredLevel)) : ilvlRatio;
        budget.ratingMultiplier = std::clamp<float>(budget.statMultiplier * (levelRatio * levelRatio), 0.15f, 1.0f);

        budget.armorMultiplier = budget.statMultiplier;
        budget.weaponDpsMultiplier = budget.statMultiplier;
        budget.spellPowerMultiplier = budget.statMultiplier;
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
                ItemModCategory const category = GetItemModCategory(proto->ItemStat[i].ItemStatType);
                float mult = budget.statMultiplier;

                if (category == ItemModCategory::SecondaryRating)
                    mult = budget.ratingMultiplier;
                else if (category == ItemModCategory::SpellPower)
                    mult = budget.spellPowerMultiplier;

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

    LOG_INFO("server.loading", ">> UniversalContentScaling: Normalized budgets for {} items with monotonic power bands",
             scaledCount);
}
