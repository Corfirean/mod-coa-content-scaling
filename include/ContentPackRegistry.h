/*
 * CoA Universal Content Scaling
 * ContentPackRegistry: Provider registry for expansion packs and content era resolution.
 */

#ifndef COA_CONTENT_PACK_REGISTRY_H
#define COA_CONTENT_PACK_REGISTRY_H

#include "ContentEra.h"
#include "ContentTier.h"
#include "Define.h"
#include "ProgressionLayout.h"
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class IEncounterAdapter;

class IContentPack
{
public:
    virtual ~IContentPack() = default;

    [[nodiscard]] virtual ContentEra GetEra() const = 0;
    [[nodiscard]] virtual std::string_view GetName() const = 0;
    [[nodiscard]] virtual LevelRange GetSourceLevelRange() const = 0;

    [[nodiscard]] virtual bool HandlesMap(uint32 mapId) const = 0;
    [[nodiscard]] virtual bool HandlesArea(uint32 areaId) const { return false; }
    [[nodiscard]] virtual bool HandlesCreature(uint32 entry) const { return false; }
    [[nodiscard]] virtual bool HandlesQuest(uint32 questId) const { return false; }
    [[nodiscard]] virtual bool HandlesItem(uint32 itemId) const { return false; }

    [[nodiscard]] virtual std::optional<ContentTier> GetEncounterTier(uint32 entry, uint32 mapId) const { return std::nullopt; }
    [[nodiscard]] virtual IEncounterAdapter const* GetEncounterAdapter(uint32 entry) const { return nullptr; }
};

class ContentPackRegistry
{
public:
    static ContentPackRegistry* Instance();

    void RegisterPack(std::shared_ptr<IContentPack> pack);
    void UnregisterPack(ContentEra era);

    // Lifecycle control: registration allowed only before finalization
    void Finalize();
    [[nodiscard]] bool IsFinalized() const { return _finalized; }

    [[nodiscard]] IContentPack const* GetPack(ContentEra era) const;
    [[nodiscard]] bool HasPack(ContentEra era) const;
    [[nodiscard]] std::vector<std::shared_ptr<IContentPack>> const& GetPacks() const { return _packs; }

    // Manual DB/config overrides
    void RegisterMapOverride(uint32 mapId, ContentEra era);
    void RegisterAreaOverride(uint32 areaId, ContentEra era);
    void RegisterCreatureOverride(uint32 entry, ContentEra era);
    void RegisterQuestOverride(uint32 questId, ContentEra era);
    void RegisterItemOverride(uint32 itemId, ContentEra era);

    // Authoritative 8-level resolution chain:
    // 1. Explicit entry override
    // 2. Explicit area override
    // 3. Explicit instance/map profile
    // 4. Registered content-pack ownership
    // 5. Authored expansion metadata
    // 6. Zone/sort metadata
    // 7. Authored level heuristic (last resort)
    static constexpr uint32 MAP_UNSPECIFIED = 0xFFFFFFFF;

    [[nodiscard]] EraResolutionResult ResolveEraDetailsForMap(uint32 mapId, uint8 authoredExpansion = 0) const;
    [[nodiscard]] EraResolutionResult ResolveEraDetailsForArea(uint32 areaId, uint32 mapId = MAP_UNSPECIFIED, uint8 authoredExpansion = 0) const;
    [[nodiscard]] EraResolutionResult ResolveEraDetailsForCreature(uint32 creatureEntry, uint32 mapId = MAP_UNSPECIFIED, uint32 areaId = 0,
                                                                   uint8 authoredExpansion = 0, uint8 authoredLevel = 0) const;
    [[nodiscard]] EraResolutionResult ResolveEraDetailsForQuest(uint32 questId, int32 zoneOrSort = 0,
                                                                uint8 authoredExpansion = 0, uint8 authoredLevel = 0) const;
    [[nodiscard]] EraResolutionResult ResolveEraDetailsForItem(uint32 itemId, uint32 itemLevel = 0,
                                                               uint32 requiredLevel = 0, uint8 authoredExpansion = 0) const;

    [[nodiscard]] ContentEra ResolveEraForMap(uint32 mapId, uint8 authoredExpansion = 0) const;
    [[nodiscard]] ContentEra ResolveEraForArea(uint32 areaId, uint32 mapId = MAP_UNSPECIFIED, uint8 authoredExpansion = 0) const;
    [[nodiscard]] ContentEra ResolveEraForCreature(uint32 creatureEntry, uint32 mapId = MAP_UNSPECIFIED, uint32 areaId = 0,
                                                   uint8 authoredExpansion = 0, uint8 authoredLevel = 0) const;
    [[nodiscard]] ContentEra ResolveEraForQuest(uint32 questId, int32 zoneOrSort = 0,
                                                uint8 authoredExpansion = 0, uint8 authoredLevel = 0) const;
    [[nodiscard]] ContentEra ResolveEraForItem(uint32 itemId, uint32 itemLevel = 0,
                                               uint32 requiredLevel = 0, uint8 authoredExpansion = 0) const;

    void Clear();

private:
    ContentPackRegistry() = default;

    bool _finalized{false};

    std::vector<std::shared_ptr<IContentPack>> _packs;
    std::unordered_map<uint32, ContentEra> _mapOverrides;
    std::unordered_map<uint32, ContentEra> _areaOverrides;
    std::unordered_map<uint32, ContentEra> _creatureOverrides;
    std::unordered_map<uint32, ContentEra> _questOverrides;
    std::unordered_map<uint32, ContentEra> _itemOverrides;
};

#define sContentPackRegistry ContentPackRegistry::Instance()

#endif // COA_CONTENT_PACK_REGISTRY_H
