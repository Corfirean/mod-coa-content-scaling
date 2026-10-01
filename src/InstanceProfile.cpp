/*
 * CoA Universal Content Scaling
 * InstanceProfile: Implementation of instance profile registry with explicit sizing.
 */

#include "InstanceProfile.h"
#include "DBCEnums.h"

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

    auto add = [this](uint32 mapId, std::string_view name, ContentEra era, bool isRaid,
                      uint32 defaultSize, uint32 r10 = 10, uint32 r25 = 25)
    {
        _profiles[mapId] = InstanceProfile{
            .mapId = mapId,
            .name = name,
            .era = era,
            .defaultGroupSize = defaultSize,
            .raid10Size = r10,
            .raid25Size = r25,
            .isRaid = isRaid
        };
    };

    // ==========================================
    // Classic Raids
    // ==========================================
    add(309, "Zul'Gurub",                 ContentEra::Classic, true, 20, 20, 20);
    add(509, "Ruins of Ahn'Qiraj",        ContentEra::Classic, true, 20, 20, 20); // AQ20 explicitly 20-man
    add(409, "Molten Core",               ContentEra::Classic, true, 40, 40, 40);
    add(469, "Blackwing Lair",            ContentEra::Classic, true, 40, 40, 40);
    add(531, "Temple of Ahn'Qiraj",       ContentEra::Classic, true, 40, 40, 40);
    add(249, "Onyxia's Lair",             ContentEra::Classic, true, 40, 10, 25);

    // ==========================================
    // Classic Dungeons
    // ==========================================
    add(33,  "Shadowfang Keep",           ContentEra::Classic, false, 5);
    add(34,  "Stormwind Stockade",        ContentEra::Classic, false, 5);
    add(36,  "The Deadmines",             ContentEra::Classic, false, 5);
    add(43,  "Wailing Caverns",           ContentEra::Classic, false, 5);
    add(47,  "Razorfen Kraul",            ContentEra::Classic, false, 5);
    add(48,  "Blackfathom Deeps",         ContentEra::Classic, false, 5);
    add(70,  "Uldaman",                   ContentEra::Classic, false, 5);
    add(90,  "Gnomeregan",                ContentEra::Classic, false, 5);
    add(109, "Sunken Temple",             ContentEra::Classic, false, 5);
    add(129, "Razorfen Downs",            ContentEra::Classic, false, 5);
    add(189, "Scarlet Monastery",         ContentEra::Classic, false, 5);
    add(209, "Zul'Farrak",                ContentEra::Classic, false, 5);
    add(229, "Blackrock Spire",           ContentEra::Classic, false, 5);
    add(230, "Blackrock Depths",          ContentEra::Classic, false, 5);
    add(289, "Scholomance",               ContentEra::Classic, false, 5);
    add(329, "Stratholme",                ContentEra::Classic, false, 5);
    add(349, "Maraudon",                  ContentEra::Classic, false, 5);
    add(389, "Ragefire Chasm",            ContentEra::Classic, false, 5);
    add(429, "Dire Maul",                 ContentEra::Classic, false, 5);

    // ==========================================
    // TBC Raids
    // ==========================================
    add(532, "Karazhan",                  ContentEra::TBC, true, 10, 10, 10);
    add(534, "Battle for Mount Hyjal",    ContentEra::TBC, true, 25, 25, 25);
    add(544, "Magtheridon's Lair",        ContentEra::TBC, true, 25, 25, 25);
    add(548, "Serpentshrine Cavern",      ContentEra::TBC, true, 25, 25, 25);
    add(550, "The Eye",                   ContentEra::TBC, true, 25, 25, 25);
    add(564, "Black Temple",              ContentEra::TBC, true, 25, 25, 25);
    add(565, "Gruul's Lair",              ContentEra::TBC, true, 25, 25, 25);
    add(568, "Zul'Aman",                  ContentEra::TBC, true, 10, 10, 10);
    add(580, "Sunwell Plateau",           ContentEra::TBC, true, 25, 25, 25);

    // ==========================================
    // TBC Dungeons
    // ==========================================
    add(269, "The Black Morass",          ContentEra::TBC, false, 5);
    add(540, "The Shattered Halls",       ContentEra::TBC, false, 5);
    add(542, "The Blood Furnace",         ContentEra::TBC, false, 5);
    add(543, "Hellfire Ramparts",         ContentEra::TBC, false, 5);
    add(545, "The Steamvault",            ContentEra::TBC, false, 5);
    add(546, "The Underbog",              ContentEra::TBC, false, 5);
    add(547, "The Slave Pens",            ContentEra::TBC, false, 5);
    add(552, "The Arcatraz",              ContentEra::TBC, false, 5);
    add(553, "The Botanica",              ContentEra::TBC, false, 5);
    add(554, "The Mechanar",              ContentEra::TBC, false, 5);
    add(555, "Shadow Labyrinth",          ContentEra::TBC, false, 5);
    add(556, "Sethekk Halls",             ContentEra::TBC, false, 5);
    add(557, "Mana-Tombs",                ContentEra::TBC, false, 5);
    add(558, "Auchenai Crypts",           ContentEra::TBC, false, 5);
    add(560, "Old Hillsbrad Foothills",   ContentEra::TBC, false, 5);
    add(585, "Magisters' Terrace",        ContentEra::TBC, false, 5);

    // ==========================================
    // WotLK Raids
    // ==========================================
    add(533, "Naxxramas",                 ContentEra::WotLK, true, 10, 10, 25);
    add(603, "Ulduar",                    ContentEra::WotLK, true, 10, 10, 25);
    add(615, "The Obsidian Sanctum",      ContentEra::WotLK, true, 10, 10, 25);
    add(616, "The Eye of Eternity",       ContentEra::WotLK, true, 10, 10, 25);
    add(624, "Vault of Archavon",         ContentEra::WotLK, true, 10, 10, 25);
    add(631, "Icecrown Citadel",          ContentEra::WotLK, true, 10, 10, 25);
    add(649, "Trial of the Crusader",     ContentEra::WotLK, true, 10, 10, 25);
    add(724, "The Ruby Sanctum",          ContentEra::WotLK, true, 10, 10, 25);

    // ==========================================
    // WotLK Dungeons
    // ==========================================
    add(574, "Utgarde Keep",              ContentEra::WotLK, false, 5);
    add(575, "Utgarde Pinnacle",          ContentEra::WotLK, false, 5);
    add(576, "The Nexus",                 ContentEra::WotLK, false, 5);
    add(578, "The Oculus",                ContentEra::WotLK, false, 5);
    add(595, "Culling of Stratholme",     ContentEra::WotLK, false, 5);
    add(599, "Halls of Stone",            ContentEra::WotLK, false, 5);
    add(600, "Drak'Tharon Keep",          ContentEra::WotLK, false, 5);
    add(601, "Azjol-Nerub",               ContentEra::WotLK, false, 5);
    add(602, "Halls of Lightning",        ContentEra::WotLK, false, 5);
    add(604, "Gundrak",                   ContentEra::WotLK, false, 5);
    add(608, "Violet Hold",               ContentEra::WotLK, false, 5);
    add(619, "Ahn'kahet: The Old Kingdom",ContentEra::WotLK, false, 5);
    add(632, "The Forge of Souls",        ContentEra::WotLK, false, 5);
    add(650, "Trial of the Champion",     ContentEra::WotLK, false, 5);
    add(658, "Pit of Saron",              ContentEra::WotLK, false, 5);
    add(668, "Halls of Reflection",       ContentEra::WotLK, false, 5);
}

InstanceProfile const* InstanceProfileRegistry::GetProfile(uint32 mapId) const
{
    auto it = _profiles.find(mapId);
    return (it != _profiles.end()) ? &it->second : nullptr;
}

std::optional<ContentEra> InstanceProfileRegistry::GetEraForMap(uint32 mapId) const
{
    if (auto const* profile = GetProfile(mapId))
        return profile->era;
    return std::nullopt;
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
