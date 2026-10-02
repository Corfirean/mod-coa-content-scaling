/*
 * CoA Universal Content Scaling
 * ProgressionContext: Unified runtime progression descriptor bridging eras, tiers, and layout.
 */

#ifndef COA_PROGRESSION_CONTEXT_H
#define COA_PROGRESSION_CONTEXT_H

#include "ContentEra.h"
#include "ContentTier.h"
#include "Define.h"
#include "ProgressionLayout.h"
#include <algorithm>
#include <cmath>
#include <string>

struct ProgressionContext
{
    uint32 authoredLevel{1};
    uint32 effectiveLevel{1};

    ContentEra era{ContentEra::Classic};
    ContentTier tier{ContentTier::WORLD};

    uint32 maxPlayerLevel{60};

    bool contentPackEnabled{true};

    // Normalized progression position across realm's active progression span [0.0f, 1.0f]
    float progressionPosition{0.0f};

    // Normalized progress within the individual content era [0.0f, 1.0f]
    float eraProgress{0.0f};

    static ProgressionContext Create(uint32 authoredLevel, ContentEra era, ContentTier tier,
                                     ProgressionLayout const& layout)
    {
        ProgressionContext ctx;
        ctx.authoredLevel = authoredLevel;
        ctx.era = era;
        ctx.tier = tier;
        ctx.maxPlayerLevel = layout.maxLevel;
        ctx.contentPackEnabled = layout.IsEraEnabled(era);

        ctx.effectiveLevel = layout.MapAuthoredToEffective(era, static_cast<uint8>(authoredLevel));

        // Calculate progression position across total level span [1, maxLevel]
        if (layout.maxLevel > 1)
        {
            ctx.progressionPosition = std::clamp<float>(
                float(ctx.effectiveLevel - 1) / float(layout.maxLevel - 1), 0.0f, 1.0f);
        }
        else
        {
            ctx.progressionPosition = 1.0f;
        }

        // Calculate eraProgress within era's own range
        LevelRange const eraRange = layout.GetEraRange(era);
        if (eraRange.maxLevel > eraRange.minLevel)
        {
            ctx.eraProgress = std::clamp<float>(
                float(ctx.effectiveLevel - eraRange.minLevel) / float(eraRange.maxLevel - eraRange.minLevel),
                0.0f, 1.0f);
        }
        else
        {
            ctx.eraProgress = 1.0f;
        }

        return ctx;
    }

    [[nodiscard]] std::string ToString() const
    {
        return "Authored: " + std::to_string(authoredLevel) +
               " -> Effective: " + std::to_string(effectiveLevel) +
               " (" + std::string(ContentEraToString(era)) + "/" + std::string(ContentTierToString(tier)) +
               ", Pos: " + std::to_string(int(progressionPosition * 100)) + "%" +
               ", EraPos: " + std::to_string(int(eraProgress * 100)) + "%)";
    }
};

#endif // COA_PROGRESSION_CONTEXT_H
