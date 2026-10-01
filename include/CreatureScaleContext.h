/*
 * CoA Universal Content Scaling
 * CreatureScaleContext: Structured context for creature level and stat scaling.
 */

#ifndef COA_CREATURE_SCALE_CONTEXT_H
#define COA_CREATURE_SCALE_CONTEXT_H

#include "ContentEra.h"
#include "ContentTier.h"
#include "Define.h"

struct CreatureScaleContext
{
    ContentEra era{ContentEra::Classic};

    uint8 authoredLevel{1};
    uint8 effectiveLevel{1};

    ContentTier tier{ContentTier::WORLD};

    bool dungeon{false};
    bool raid{false};
    bool boss{false};
    bool worldBoss{false};
    bool heroic{false};

    // Scaling multipliers
    float healthModifier{1.0f};
    float damageModifier{1.0f};
    float armorModifier{1.0f};
    float manaModifier{1.0f};
    float spellModifier{1.0f};

    // Group scaling multiplier
    float groupHealthScale{1.0f};
    float groupDamageScale{1.0f};
};

#endif // COA_CREATURE_SCALE_CONTEXT_H
