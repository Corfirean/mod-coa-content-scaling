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
    if (!adapter)
        return;

    EncounterAdapterKey const key{ adapter->GetMapId(), adapter->GetEncounterId() };
    _adapters[key] = adapter;
    _legacyAdapters[adapter->GetEncounterId()] = adapter;
}

IEncounterAdapter const* AdaptiveEncounterMgr::GetAdapter(uint32 mapId, uint32 encounterId) const
{
    auto it = _adapters.find(EncounterAdapterKey{ mapId, encounterId });
    if (it != _adapters.end())
        return it->second.get();

    return GetAdapter(encounterId);
}

IEncounterAdapter const* AdaptiveEncounterMgr::GetAdapter(uint32 encounterId) const
{
    auto it = _legacyAdapters.find(encounterId);
    return (it != _legacyAdapters.end()) ? it->second.get() : nullptr;
}

uint32 AdaptiveEncounterMgr::ResolveMechanic(uint32 mapId, uint32 encounterId, uint32 mechanicId,
                                             EncounterMechanicType type, uint32 authoredValue,
                                             EncounterContext const& ctx) const
{
    // Full-group parity: If mechanics participants meets or exceeds intended authored size and no challenge downscale, return authored
    if (ctx.mechanicParticipants >= ctx.intendedPlayers && ctx.challengeSize == 0)
        return authoredValue;

    if (auto const* adapter = GetAdapter(mapId, encounterId))
        return adapter->ResolveMechanic(mechanicId, type, authoredValue, ctx);

    // Generic default scaling based on mechanic type
    switch (type)
    {
        case EncounterMechanicType::TARGET_COUNT:
            return ScaleTargetCount(encounterId, authoredValue, ctx);
        case EncounterMechanicType::ADD_COUNT:
            return ScaleAddCount(encounterId, authoredValue, ctx);
        case EncounterMechanicType::REQUIRED_PLAYERS:
        case EncounterMechanicType::REQUIRED_INTERACTORS:
            return ScaleRequiredPlayers(encounterId, authoredValue, ctx);
        case EncounterMechanicType::WAVE_SIZE:
        {
            if (ctx.isMechanicSolo)
                return std::max(1u, authoredValue / 3);
            float const ratio = float(ctx.mechanicParticipants) / float(std::max(1u, ctx.intendedPlayers));
            return std::max(1u, static_cast<uint32>(std::round(float(authoredValue) * ratio)));
        }
        default:
            return authoredValue;
    }
}

uint32 AdaptiveEncounterMgr::ScaleTargetCount(uint32 encounterId, uint32 authoredTargets, EncounterContext const& ctx) const
{
    if (ctx.mechanicParticipants >= ctx.intendedPlayers && ctx.challengeSize == 0)
        return authoredTargets;

    if (auto const* adapter = GetAdapter(ctx.mapId, encounterId))
        return adapter->ScaleTargetCount(authoredTargets, ctx);

    if (ctx.isMechanicSolo)
        return 1;

    float const ratio = float(ctx.mechanicParticipants) / float(std::max(1u, ctx.intendedPlayers));
    return std::max(1u, static_cast<uint32>(std::round(float(authoredTargets) * ratio)));
}

uint32 AdaptiveEncounterMgr::ScaleAddCount(uint32 encounterId, uint32 authoredAdds, EncounterContext const& ctx) const
{
    if (ctx.mechanicParticipants >= ctx.intendedPlayers && ctx.challengeSize == 0)
        return authoredAdds;

    if (auto const* adapter = GetAdapter(ctx.mapId, encounterId))
        return adapter->ScaleAddCount(authoredAdds, ctx);

    if (ctx.isMechanicSolo)
        return std::max(1u, authoredAdds / 3);

    float const ratio = float(ctx.mechanicParticipants) / float(std::max(1u, ctx.intendedPlayers));
    return std::max(1u, static_cast<uint32>(std::round(float(authoredAdds) * ratio)));
}

uint32 AdaptiveEncounterMgr::ScaleRequiredPlayers(uint32 encounterId, uint32 authoredRequirement, EncounterContext const& ctx) const
{
    if (ctx.mechanicParticipants >= ctx.intendedPlayers && ctx.challengeSize == 0)
        return authoredRequirement;

    if (auto const* adapter = GetAdapter(ctx.mapId, encounterId))
        return adapter->ScaleRequiredPlayers(authoredRequirement, ctx);

    if (ctx.isMechanicSolo)
        return 1;

    return std::min(authoredRequirement, std::max(1u, ctx.mechanicParticipants));
}

EncounterCompatibility AdaptiveEncounterMgr::ClassifyEncounter(uint32 mapId, uint32 encounterId) const
{
    if (auto const* adapter = GetAdapter(mapId, encounterId))
        return adapter->GetCompatibility();

    return EncounterCompatibility::AUTO;
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
    _legacyAdapters.clear();
}
