/*
 * CoA Universal Content Scaling
 * InstanceProfile: Implementation of instance profile registry with explicit sizing.
 */

#include "InstanceProfile.h"
#include "CoAContentScaling.h"
#include "DBCEnums.h"
#include "GeneratedContentCensus.h"

InstanceProfileRegistry* InstanceProfileRegistry::Instance()
{
    static InstanceProfileRegistry instance;
    return &instance;
}

InstanceProfileRegistry::InstanceProfileRegistry()
{
    Initialize();
}

void InstanceProfileRegistry::Initialize()
{
    _profiles.clear();

    // Generated-first: Populate authoritative PvE instance profiles from generated census
    for (auto const& gp : sGeneratedInstanceProfiles)
    {
        // Enforce strict PvP exclusion: Battlegrounds and Arenas must NEVER enter PvE registry
        if (!IsPvEInstanceKind(gp.kind))
            continue;

        auto it = _profiles.find(gp.mapId);
        if (it == _profiles.end())
        {
            _profiles[gp.mapId] = InstanceProfile{
                .mapId = gp.mapId,
                .name = gp.name,
                .era = gp.era,
                .tier = gp.tier,
                .kind = gp.kind,
                .defaultGroupSize = gp.intendedPlayers,
                .raid10Size = (gp.intendedPlayers == 10) ? 10u : 10u,
                .raid25Size = (gp.intendedPlayers >= 20) ? gp.intendedPlayers : 25u,
                .isRaid = gp.isRaid
            };
        }
        else
        {
            // For multi-variant instances (e.g. 25-man raid variant with diff 1), calibrate 25-man size
            if (gp.difficulty == 1 && gp.isRaid)
                it->second.raid25Size = gp.intendedPlayers;
        }
    }
}

InstanceProfile const* InstanceProfileRegistry::GetProfile(uint32 mapId) const
{
    auto it = _profiles.find(mapId);
    return (it != _profiles.end()) ? &it->second : nullptr;
}

std::optional<ContentEra> InstanceProfileRegistry::GetEraForMap(uint32 mapId, uint8 difficulty) const
{
    if (mapId == 249)
    {
        // Reused Onyxia: 25-man (1, 3) and 10-man heroic (2) are WotLK era.
        if (difficulty == RAID_DIFFICULTY_25MAN_NORMAL ||
            difficulty == RAID_DIFFICULTY_10MAN_HEROIC ||
            difficulty == RAID_DIFFICULTY_25MAN_HEROIC)
            return ContentEra::WotLK;

        // Difficulty 0 is 10-man normal in WotLK, or 40-man Classic when WotLK is inactive
        if (difficulty == RAID_DIFFICULTY_10MAN_NORMAL)
            return sCoAContentScaling->IsWotlkEnabled() ? ContentEra::WotLK : ContentEra::Classic;

        return ContentEra::Classic;
    }

    if (auto const* profile = GetProfile(mapId))
        return profile->era;
    return std::nullopt;
}

ContentTier InstanceProfileRegistry::GetTierForMap(uint32 mapId, uint8 difficulty) const
{
    if (auto const* profile = GetProfile(mapId))
    {
        if (!profile->isRaid && (difficulty == DUNGEON_DIFFICULTY_HEROIC))
            return ContentTier::DUNGEON_HEROIC;
        return profile->tier;
    }
    return ContentTier::WORLD;
}

uint32 InstanceProfileRegistry::GetIntendedPlayers(uint32 mapId, uint8 difficulty) const
{
    auto const* profile = GetProfile(mapId);
    if (!profile)
        return 5;

    if (!profile->isRaid)
        return profile->defaultGroupSize;

    if (profile->era == ContentEra::Classic)
    {
        // Onyxia map 249: difficulty 10/25 in WotLK, otherwise 40
        if (mapId == 249 && (difficulty == RAID_DIFFICULTY_10MAN_NORMAL || difficulty == RAID_DIFFICULTY_10MAN_HEROIC))
            return 10;
        if (mapId == 249 && (difficulty == RAID_DIFFICULTY_25MAN_NORMAL || difficulty == RAID_DIFFICULTY_25MAN_HEROIC))
            return 25;

        return profile->defaultGroupSize;
    }

    if (difficulty == RAID_DIFFICULTY_10MAN_NORMAL || difficulty == RAID_DIFFICULTY_10MAN_HEROIC)
        return profile->raid10Size;

    return profile->raid25Size;
}

MapContentKind InstanceProfileRegistry::GetKindForMap(uint32 mapId) const
{
    if (auto const* mapProf = FindGeneratedMapProfile(mapId))
        return mapProf->kind;
    if (auto const* prof = GetProfile(mapId))
        return prof->kind;
    return MapContentKind::UNKNOWN;
}

bool InstanceProfileRegistry::HasProfile(uint32 mapId) const
{
    return _profiles.count(mapId) > 0;
}

bool InstanceProfileRegistry::ValidateAll(std::vector<std::string>& issues) const
{
    issues.clear();
    for (auto const& [mapId, prof] : _profiles)
    {
        // Enforce strict PvP exclusion
        if (prof.kind == MapContentKind::BATTLEGROUND || prof.kind == MapContentKind::ARENA)
            issues.push_back("PvP map " + std::to_string(mapId) + " (" + std::string(prof.name) + ") found in PvE InstanceProfileRegistry");

        if (prof.defaultGroupSize == 0)
            issues.push_back("Map " + std::to_string(mapId) + " (" + std::string(prof.name) + "): defaultGroupSize is 0");

        if (prof.isRaid)
        {
            if (prof.era == ContentEra::Classic && prof.defaultGroupSize != 10 && prof.defaultGroupSize != 20 && prof.defaultGroupSize != 25 && prof.defaultGroupSize != 40)
                issues.push_back("Classic raid " + std::string(prof.name) + " has unexpected size: " + std::to_string(prof.defaultGroupSize));
            else if (prof.era == ContentEra::TBC && prof.defaultGroupSize != 10 && prof.defaultGroupSize != 25)
                issues.push_back("TBC raid " + std::string(prof.name) + " has non-standard raid sizing");
            else if (prof.era == ContentEra::WotLK && (prof.raid10Size != 10 || prof.raid25Size != 25))
                issues.push_back("WotLK raid " + std::string(prof.name) + " has non-standard raid sizing");
        }
    }
    return issues.empty();
}
