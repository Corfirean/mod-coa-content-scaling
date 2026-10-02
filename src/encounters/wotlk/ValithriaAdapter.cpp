/*
 * CoA Universal Content Scaling
 * ValithriaAdapter: Adapts dream portal count and enables alternative healing contributions for solo/small groups.
 * Map: 631 (Icecrown Citadel), Encounter: DATA_VALITHRIA_DREAMWALKER (10)
 */

#include "AdaptiveEncounterAPI.h"

class ValithriaAdapter : public IEncounterAdapter
{
public:
    [[nodiscard]] uint32 GetMapId() const override { return 631; }
    [[nodiscard]] uint32 GetEncounterId() const override { return 10; } // DATA_VALITHRIA_DREAMWALKER
    [[nodiscard]] std::string_view GetName() const override { return "Valithria Dreamwalker"; }

    [[nodiscard]] uint32 ResolveMechanic(uint32 mechanicId, EncounterMechanicType type,
                                         uint32 authoredValue, EncounterContext const& ctx) const override
    {
        // Full group parity
        if (ctx.mechanicParticipants >= ctx.intendedPlayers && ctx.challengeSize == 0)
            return authoredValue;

        switch (type)
        {
            case EncounterMechanicType::OBJECTIVE_COUNT:
            case EncounterMechanicType::REQUIRED_INTERACTORS:
            {
                // Number of Dream Portals spawned per wave (authored: 3 in 10-man, 8 in 25-man)
                // For solo/small group, match the available player count so no portals are wasted
                // and missed portal counters don't punish.
                if (ctx.isMechanicSolo)
                    return 1;
                return std::min<uint32>(authoredValue, ctx.mechanicParticipants);
            }
            case EncounterMechanicType::HEALING_CONTRIBUTION:
            {
                // Percent heal given to Valithria upon defeating priority adds (e.g. Suppresser / Archmage)
                // In full groups: 0 (authored requires direct healing).
                // In solo / non-healer small groups: 5% - 10% per wave slain, enabling non-healers to clear encounter.
                if (ctx.isMechanicSolo)
                    return 8; // 8% per key add kill
                if (ctx.mechanicParticipants <= 2)
                    return 4; // 4% per key add kill
                return 0;
            }
            case EncounterMechanicType::ADD_COUNT:
            case EncounterMechanicType::WAVE_SIZE:
            {
                // Suppresser and Blazing Skeleton add wave counts
                if (ctx.isMechanicSolo)
                    return std::max(1u, authoredValue / 3);
                float const ratio = float(ctx.mechanicParticipants) / float(std::max(1u, ctx.intendedPlayers));
                return std::max(1u, static_cast<uint32>(std::round(float(authoredValue) * ratio)));
            }
            default:
                return IEncounterAdapter::ResolveMechanic(mechanicId, type, authoredValue, ctx);
        }
    }
};

std::shared_ptr<IEncounterAdapter> CreateValithriaAdapter()
{
    return std::make_shared<ValithriaAdapter>();
}
