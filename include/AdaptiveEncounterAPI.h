/*
 * CoA Universal Content Scaling
 * AdaptiveEncounterAPI: Dynamic encounter mechanics adaptation for scalable group sizes.
 */

#ifndef COA_ADAPTIVE_ENCOUNTER_API_H
#define COA_ADAPTIVE_ENCOUNTER_API_H

#include "Define.h"
#include <memory>
#include <string_view>
#include <unordered_map>

enum class EncounterCompatibility : uint8
{
    AUTO            = 0,
    NEEDS_OVERRIDE  = 1,
    OVERRIDDEN      = 2,
    UNSUPPORTED     = 3
};

constexpr std::string_view CompatibilityToString(EncounterCompatibility comp)
{
    switch (comp)
    {
        case EncounterCompatibility::AUTO:           return "AUTO";
        case EncounterCompatibility::NEEDS_OVERRIDE: return "NEEDS_OVERRIDE";
        case EncounterCompatibility::OVERRIDDEN:     return "OVERRIDDEN";
        case EncounterCompatibility::UNSUPPORTED:    return "UNSUPPORTED";
        default:                                     return "UNKNOWN";
    }
}

struct EncounterContext
{
    uint32 encounterId{0};
    uint32 mapId{0};
    uint32 instanceId{0};
    float effectivePlayers{1.0f};
    uint32 intendedPlayers{5};
    bool isSolo{false};
};

class IEncounterAdapter
{
public:
    virtual ~IEncounterAdapter() = default;

    [[nodiscard]] virtual uint32 GetEncounterId() const = 0;
    [[nodiscard]] virtual std::string_view GetName() const = 0;
    [[nodiscard]] virtual EncounterCompatibility GetCompatibility() const { return EncounterCompatibility::OVERRIDDEN; }

    [[nodiscard]] virtual uint32 ScaleTargetCount(uint32 authoredTargets, EncounterContext const& ctx) const
    {
        if (ctx.isSolo)
            return 1;
        float const ratio = ctx.effectivePlayers / float(std::max(1u, ctx.intendedPlayers));
        return std::max(1u, static_cast<uint32>(std::round(float(authoredTargets) * ratio)));
    }

    [[nodiscard]] virtual uint32 ScaleAddCount(uint32 authoredAdds, EncounterContext const& ctx) const
    {
        if (ctx.isSolo)
            return std::max(1u, authoredAdds / 3);
        float const ratio = ctx.effectivePlayers / float(std::max(1u, ctx.intendedPlayers));
        return std::max(1u, static_cast<uint32>(std::round(float(authoredAdds) * ratio)));
    }

    [[nodiscard]] virtual uint32 ScaleRequiredPlayers(uint32 authoredRequirement, EncounterContext const& ctx) const
    {
        if (ctx.isSolo)
            return 1;
        return std::min(authoredRequirement, static_cast<uint32>(std::ceil(ctx.effectivePlayers)));
    }
};

class AdaptiveEncounterMgr
{
public:
    static AdaptiveEncounterMgr* Instance();

    void RegisterAdapter(std::shared_ptr<IEncounterAdapter> adapter);
    [[nodiscard]] IEncounterAdapter const* GetAdapter(uint32 encounterId) const;

    [[nodiscard]] uint32 ScaleTargetCount(uint32 encounterId, uint32 authoredTargets, EncounterContext const& ctx) const;
    [[nodiscard]] uint32 ScaleAddCount(uint32 encounterId, uint32 authoredAdds, EncounterContext const& ctx) const;
    [[nodiscard]] uint32 ScaleRequiredPlayers(uint32 encounterId, uint32 authoredRequirement, EncounterContext const& ctx) const;

    [[nodiscard]] EncounterCompatibility ClassifyEncounter(uint32 encounterId) const;

    void Clear();

private:
    AdaptiveEncounterMgr() = default;
    std::unordered_map<uint32, std::shared_ptr<IEncounterAdapter>> _adapters;
};

#define sAdaptiveEncounterMgr AdaptiveEncounterMgr::Instance()

#endif // COA_ADAPTIVE_ENCOUNTER_API_H
