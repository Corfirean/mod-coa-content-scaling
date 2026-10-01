/*
 * CoA Universal Content Scaling
 * CoATBCContent: The Burning Crusade content pack provider.
 */

#ifndef COA_TBC_CONTENT_H
#define COA_TBC_CONTENT_H

#include "ContentPackRegistry.h"

class CoATBCContentPack : public IContentPack
{
public:
    CoATBCContentPack() = default;

    [[nodiscard]] ContentEra GetEra() const override { return ContentEra::TBC; }
    [[nodiscard]] std::string_view GetName() const override { return "The Burning Crusade"; }
    [[nodiscard]] LevelRange GetSourceLevelRange() const override { return LevelRange(58, 70); }

    [[nodiscard]] bool HandlesMap(uint32 mapId) const override;
    [[nodiscard]] bool HandlesArea(uint32 areaId) const override;
    [[nodiscard]] bool HandlesCreature(uint32 entry) const override;
    [[nodiscard]] bool HandlesQuest(uint32 questId) const override;
    [[nodiscard]] bool HandlesItem(uint32 itemId) const override;
};

#endif // COA_TBC_CONTENT_H
