/*
 * CoA Universal Content Scaling
 * ContentPackRegistry: Implementation of registry and hierarchical era resolution.
 */

#include "ContentPackRegistry.h"
#include <algorithm>

ContentPackRegistry* ContentPackRegistry::Instance()
{
    static ContentPackRegistry instance;
    return &instance;
}

void ContentPackRegistry::RegisterPack(std::shared_ptr<IContentPack> pack)
{
    if (!pack)
        return;

    UnregisterPack(pack->GetEra());
    _packs.push_back(pack);
}

void ContentPackRegistry::UnregisterPack(ContentEra era)
{
    _packs.erase(
        std::remove_if(_packs.begin(), _packs.end(),
            [era](std::shared_ptr<IContentPack> const& p) { return p->GetEra() == era; }),
        _packs.end());
}

IContentPack const* ContentPackRegistry::GetPack(ContentEra era) const
{
    for (auto const& pack : _packs)
        if (pack->GetEra() == era)
            return pack.get();
    return nullptr;
}

bool ContentPackRegistry::HasPack(ContentEra era) const
{
    return GetPack(era) != nullptr;
}

void ContentPackRegistry::RegisterMapOverride(uint32 mapId, ContentEra era)
{
    _mapOverrides[mapId] = era;
}

void ContentPackRegistry::RegisterAreaOverride(uint32 areaId, ContentEra era)
{
    _areaOverrides[areaId] = era;
}

void ContentPackRegistry::RegisterCreatureOverride(uint32 entry, ContentEra era)
{
    _creatureOverrides[entry] = era;
}

void ContentPackRegistry::RegisterQuestOverride(uint32 questId, ContentEra era)
{
    _questOverrides[questId] = era;
}

void ContentPackRegistry::RegisterItemOverride(uint32 itemId, ContentEra era)
{
    _itemOverrides[itemId] = era;
}

ContentEra ContentPackRegistry::ResolveEraForMap(uint32 mapId, uint8 authoredExpansion) const
{
    // 1. Explicit override
    auto it = _mapOverrides.find(mapId);
    if (it != _mapOverrides.end())
        return it->second;

    // 2. Query registered content packs
    for (auto const& pack : _packs)
        if (pack->HandlesMap(mapId))
            return pack->GetEra();

    // 3. Fallback based on canonical maps
    if (mapId == 530) // Outland
        return ContentEra::TBC;
    if (mapId == 571) // Northrend
        return ContentEra::WotLK;

    // 4. Authored expansion hint
    if (authoredExpansion == 1)
        return ContentEra::TBC;
    if (authoredExpansion == 2)
        return ContentEra::WotLK;

    return ContentEra::Classic;
}

ContentEra ContentPackRegistry::ResolveEraForArea(uint32 areaId, uint32 mapId, uint8 authoredExpansion) const
{
    // 1. Explicit override
    auto it = _areaOverrides.find(areaId);
    if (it != _areaOverrides.end())
        return it->second;

    // 2. Query registered content packs
    for (auto const& pack : _packs)
        if (pack->HandlesArea(areaId))
            return pack->GetEra();

    // 3. Defer to map resolution
    if (mapId > 0)
        return ResolveEraForMap(mapId, authoredExpansion);

    return (authoredExpansion == 2) ? ContentEra::WotLK :
           (authoredExpansion == 1) ? ContentEra::TBC : ContentEra::Classic;
}

ContentEra ContentPackRegistry::ResolveEraForCreature(uint32 creatureEntry, uint32 mapId, uint32 areaId,
                                                   uint8 authoredExpansion, uint8 authoredLevel) const
{
    // 1. Explicit creature entry override
    auto it = _creatureOverrides.find(creatureEntry);
    if (it != _creatureOverrides.end())
        return it->second;

    // 2. Query registered content packs for specific creature entry
    for (auto const& pack : _packs)
        if (pack->HandlesCreature(creatureEntry))
            return pack->GetEra();

    // 3. If area / map is specific to an expansion
    if (mapId > 0)
    {
        ContentEra const mapEra = ResolveEraForMap(mapId, authoredExpansion);
        if (mapEra != ContentEra::Classic)
            return mapEra;
    }

    if (areaId > 0)
    {
        ContentEra const areaEra = ResolveEraForArea(areaId, mapId, authoredExpansion);
        if (areaEra != ContentEra::Classic)
            return areaEra;
    }

    // 4. Authored expansion metadata from creature_template.exp
    if (authoredExpansion == 2 || authoredLevel >= 68)
        return ContentEra::WotLK;
    if (authoredExpansion == 1 || (authoredLevel >= 58 && authoredLevel <= 70))
        return ContentEra::TBC;

    return ContentEra::Classic;
}

ContentEra ContentPackRegistry::ResolveEraForQuest(uint32 questId, int32 zoneOrSort,
                                                uint8 authoredExpansion, uint8 authoredLevel) const
{
    // 1. Explicit override
    auto it = _questOverrides.find(questId);
    if (it != _questOverrides.end())
        return it->second;

    // 2. Query registered content packs
    for (auto const& pack : _packs)
        if (pack->HandlesQuest(questId))
            return pack->GetEra();

    // 3. ZoneOrSort hints (e.g. Outland / Northrend zones)
    // If authored level indicates era
    if (authoredExpansion == 2 || authoredLevel >= 68)
        return ContentEra::WotLK;
    if (authoredExpansion == 1 || (authoredLevel >= 58 && authoredLevel <= 70))
        return ContentEra::TBC;

    return ContentEra::Classic;
}

ContentEra ContentPackRegistry::ResolveEraForItem(uint32 itemId, uint32 itemLevel,
                                               uint32 requiredLevel, uint8 authoredExpansion) const
{
    // 1. Explicit override
    auto it = _itemOverrides.find(itemId);
    if (it != _itemOverrides.end())
        return it->second;

    // 2. Query registered content packs
    for (auto const& pack : _packs)
        if (pack->HandlesItem(itemId))
            return pack->GetEra();

    // 3. ilvl / req level thresholds
    if (authoredExpansion == 2 || requiredLevel >= 68 || itemLevel >= 150)
        return ContentEra::WotLK;
    if (authoredExpansion == 1 || requiredLevel >= 58 || itemLevel >= 85)
        return ContentEra::TBC;

    return ContentEra::Classic;
}

void ContentPackRegistry::Clear()
{
    _packs.clear();
    _mapOverrides.clear();
    _areaOverrides.clear();
    _creatureOverrides.clear();
    _questOverrides.clear();
    _itemOverrides.clear();
}
