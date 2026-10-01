/*
 * CoA Universal Content Scaling
 * AdaptiveEncounterAPI: Implementation of dynamic encounter adaptation.
 */

#include "AdaptiveEncounterAPI.h"
#include <algorithm>
#include <cmath>

AdaptiveEncounterMgr* AdaptiveEncounterMgr::Instance()
{
    static AdaptiveEncounterMgr instance;
    return &instance;
}

void AdaptiveEncounterMgr::RegisterAdapter(std::shared_ptr<IEncounterAdapter> adapter)
{
    if (adapter)
        _adapters[adapter->GetEncounterId()] = adapter;
}

IEncounterAdapter const* AdaptiveEncounterMgr::GetAdapter(uint32 encounterId) const
{
    auto it = _adapters.find(encounterId);
    return (it != _adapters.end()) ? it->second.get() : nullptr;
}

uint32 AdaptiveEncounterMgr::ScaleTargetCount(uint32 encounterId, uint32 authoredTargets, EncounterContext const& ctx) const
{
    if (auto const* adapter = GetAdapter(encounterId))
        return adapter->ScaleTargetCount(authoredTargets, ctx);

    if (ctx.isSolo)
        return 1;

    float const ratio = ctx.effectivePlayers / float(std::max(1u, ctx.intendedPlayers));
    return std::max(1u, static_cast<uint32>(std::round(float(authoredTargets) * ratio)));
}

uint32 AdaptiveEncounterMgr::ScaleAddCount(uint32 encounterId, uint32 authoredAdds, EncounterContext const& ctx) const
{
    if (auto const* adapter = GetAdapter(encounterId))
        return adapter->ScaleAddCount(authoredAdds, ctx);

    if (ctx.isSolo)
        return std::max(1u, authoredAdds / 3);

    float const ratio = ctx.effectivePlayers / float(std::max(1u, ctx.intendedPlayers));
    return std::max(1u, static_cast<uint32>(std::round(float(authoredAdds) * ratio)));
}

uint32 AdaptiveEncounterMgr::ScaleRequiredPlayers(uint32 encounterId, uint32 authoredRequirement, EncounterContext const& ctx) const
{
    if (auto const* adapter = GetAdapter(encounterId))
        return adapter->ScaleRequiredPlayers(authoredRequirement, ctx);

    if (ctx.isSolo)
        return 1;

    return std::min(authoredRequirement, static_cast<uint32>(std::ceil(ctx.effectivePlayers)));
}

EncounterCompatibility AdaptiveEncounterMgr::ClassifyEncounter(uint32 encounterId) const
{
    if (auto const* adapter = GetAdapter(encounterId))
        return adapter->GetCompatibility();

    return EncounterCompatibility::AUTO;
}

void AdaptiveEncounterMgr::Clear()
{
    _adapters.clear();
}
