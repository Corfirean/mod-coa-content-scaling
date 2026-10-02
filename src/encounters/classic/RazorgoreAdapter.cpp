/*
 * CoA Universal Content Scaling
 * RazorgoreAdapter: Adapts Phase 1 egg destruction and add spawn rates for small groups.
 * Map: 469 (Blackwing Lair), Encounter: DATA_RAZORGORE_THE_UNTAMED (0)
 */

#include "AdaptiveEncounterAPI.h"

class RazorgoreAdapter : public IEncounterAdapter
{
public:
    [[nodiscard]] uint32 GetMapId() const override { return 469; }
    [[nodiscard]] uint32 GetEncounterId() const override { return 0; } // DATA_RAZORGORE_THE_UNTAMED
    [[nodiscard]] std::string_view GetName() const override { return "Razorgore the Untamed"; }

    [[nodiscard]] uint32 ResolveMechanic(uint32 mechanicId, EncounterMechanicType type,
                                         uint32 authoredValue, EncounterContext const& ctx) const override
    {
        // Full group parity
        if (ctx.mechanicParticipants >= ctx.intendedPlayers && ctx.challengeSize == 0)
            return authoredValue;

        switch (type)
        {
            case EncounterMechanicType::OBJECTIVE_COUNT:
            {
                // Egg count to destroy before Phase 2 triggers:
                // Authored is typically 30 eggs (EggList.size()).
                // For solo: scale down to 6-10 eggs so 1 player doesn't wipe to endless adds.
                // For 2-4 players: scale proportionally.
                if (ctx.isMechanicSolo)
                    return std::max<uint32>(6, authoredValue / 4);

                float const ratio = float(ctx.mechanicParticipants) / float(std::max(1u, ctx.intendedPlayers));
                return std::max<uint32>(6, static_cast<uint32>(std::round(float(authoredValue) * ratio)));
            }
            case EncounterMechanicType::TIMER_MS:
            {
                // Mind Exhaustion debuff duration (authored 60000ms):
                // In solo mode, player has no alternate charmer to rotate.
                // Reduce to 0ms (or 3000ms) so solo player can re-channel orb.
                if (ctx.isMechanicSolo)
                    return 0; // Remove exhaustion cooldown entirely for solo
                if (ctx.mechanicParticipants <= 2)
                    return 15000; // 15s for 2 players
                return authoredValue;
            }
            case EncounterMechanicType::ADD_COUNT:
            case EncounterMechanicType::WAVE_SIZE:
            {
                // Add wave frequency / size in Phase 1
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

std::shared_ptr<IEncounterAdapter> CreateRazorgoreAdapter()
{
    return std::make_shared<RazorgoreAdapter>();
}
