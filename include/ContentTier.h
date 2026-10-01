/*
 * CoA Universal Content Scaling
 * ContentTier: Semantic power tier distinguishing character level from content difficulty.
 */

#ifndef COA_CONTENT_TIER_H
#define COA_CONTENT_TIER_H

#include "Define.h"
#include <string_view>

enum class ContentTier : uint8
{
    WORLD           = 0,
    DUNGEON_NORMAL  = 1,
    DUNGEON_HEROIC  = 2,
    RAID_ENTRY      = 3,
    RAID_MID        = 4,
    RAID_END        = 5,
    RAID_PINNACLE   = 6
};

constexpr std::string_view ContentTierToString(ContentTier tier)
{
    switch (tier)
    {
        case ContentTier::WORLD:          return "WORLD";
        case ContentTier::DUNGEON_NORMAL: return "DUNGEON_NORMAL";
        case ContentTier::DUNGEON_HEROIC: return "DUNGEON_HEROIC";
        case ContentTier::RAID_ENTRY:     return "RAID_ENTRY";
        case ContentTier::RAID_MID:       return "RAID_MID";
        case ContentTier::RAID_END:       return "RAID_END";
        case ContentTier::RAID_PINNACLE:  return "RAID_PINNACLE";
        default:                          return "UNKNOWN";
    }
}

#endif // COA_CONTENT_TIER_H
