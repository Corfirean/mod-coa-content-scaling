/*
 * CoA Universal Content Scaling
 * CoAWotLKContent: Wrath of the Lich King content pack provider.
 */

#ifndef COA_WOTLK_CONTENT_H
#define COA_WOTLK_CONTENT_H

#include "ContentPackRegistry.h"

class CoAWotLKContentPack : public IContentPack
{
public:
    CoAWotLKContentPack() = default;

    [[nodiscard]] ContentEra GetEra() const override { return ContentEra::WotLK; }
    [[nodiscard]] std::string_view GetName() const override { return "Wrath of the Lich King"; }
    [[nodiscard]] LevelRange GetSourceLevelRange() const override { return LevelRange(68, 80); }

    [[nodiscard]] bool HandlesMap(uint32 mapId) const override;
    [[nodiscard]] bool HandlesArea(uint32 areaId) const override;
    [[nodiscard]] bool HandlesCreature(uint32 entry) const override;
    [[nodiscard]] bool HandlesQuest(uint32 questId) const override;
    [[nodiscard]] bool HandlesItem(uint32 itemId) const override;
};

#endif // COA_WOTLK_CONTENT_H
