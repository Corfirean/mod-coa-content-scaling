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

#endif // COA_CONTENT_ERA_H
