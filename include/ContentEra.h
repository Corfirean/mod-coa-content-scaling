/*
 * CoA Universal Content Scaling
 * ContentEra: Enumeration and identification of content progression eras.
 */

#ifndef COA_CONTENT_ERA_H
#define COA_CONTENT_ERA_H

#include "Define.h"
#include <string_view>

enum class ContentEra : uint8
{
    Classic = 0,
    TBC     = 1,
    WotLK   = 2,
    Custom  = 3
};

constexpr std::string_view ContentEraToString(ContentEra era)
{
    switch (era)
    {
        case ContentEra::Classic: return "Classic";
        case ContentEra::TBC:     return "TBC";
        case ContentEra::WotLK:   return "WotLK";
        case ContentEra::Custom:  return "Custom";
        default:                  return "Unknown";
    }
}

enum class EraResolutionSource : uint8
{
    ExplicitOverride       = 0,
    ExplicitAreaOverride   = 1,
    InstanceProfile        = 2,
    ContentPack            = 3,
    AuthoredExpansion      = 4,
    ZoneSortMetadata       = 5,
    AuthoredLevelHeuristic = 6,
    SafeFallback           = 7
};

constexpr std::string_view EraResolutionSourceToString(EraResolutionSource source)
{
    switch (source)
    {
        case EraResolutionSource::ExplicitOverride:       return "ExplicitOverride";
        case EraResolutionSource::ExplicitAreaOverride:   return "ExplicitAreaOverride";
        case EraResolutionSource::InstanceProfile:        return "InstanceProfile";
        case EraResolutionSource::ContentPack:            return "ContentPack";
        case EraResolutionSource::AuthoredExpansion:      return "AuthoredExpansion";
        case EraResolutionSource::ZoneSortMetadata:       return "ZoneSortMetadata";
        case EraResolutionSource::AuthoredLevelHeuristic: return "AuthoredLevelHeuristic";
        case EraResolutionSource::SafeFallback:           return "SafeFallback";
        default:                                          return "Unknown";
    }
}

struct EraResolutionResult
{
    ContentEra era{ContentEra::Classic};
    EraResolutionSource source{EraResolutionSource::SafeFallback};
    float confidence{1.0f};
};

#endif // COA_CONTENT_ERA_H
