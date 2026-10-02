/*
 * CoA Universal Content Scaling
 * FourHorsemenAdapter: Adapts mark timers, punishment bolt suppression, and simultaneous tank demands.
 * Map: 533 (Naxxramas), Encounter: BOSS_HORSEMAN (12)
 */

#include "AdaptiveEncounterAPI.h"

class FourHorsemenAdapter : public IEncounterAdapter
{
public:
    [[nodiscard]] uint32 GetMapId() const override { return 533; }
    [[nodiscard]] uint32 GetEncounterId() const override { return 12; } // BOSS_HORSEMAN
    [[nodiscard]] std::string_view GetName() const override { return "The Four Horsemen"; }

    [[nodiscard]] uint32 ResolveMechanic(uint32 mechanicId, EncounterMechanicType type,
                                         uint32 authoredValue, EncounterContext const& ctx) const override
    {
        // Full group parity
        if (ctx.mechanicParticipants >= ctx.intendedPlayers && ctx.challengeSize == 0)
            return authoredValue;

        switch (type)
        {
            case EncounterMechanicType::REQUIRED_PLAYERS:
            {
                // Number of simultaneous tanking positions required (authored: 4)
                if (ctx.isMechanicSolo)
                    return 1;
                return std::min<uint32>(authoredValue, ctx.mechanicParticipants);
            }
            case EncounterMechanicType::TIMER_MS:
            {
                // Mark interval timer (authored: 12000ms / 15000ms):
                // For small groups unable to rotate 4 corners, lengthen mark timer proportionally
                // so stacks drop off before lethality.
                if (ctx.isMechanicSolo)
                    return authoredValue * 3; // 36s - 45s: allows engaging bosses sequentially without mark stack death
                if (ctx.mechanicParticipants <= 2)
                    return authoredValue * 2; // 24s - 30s
                return authoredValue;
            }
            case EncounterMechanicType::FAIL_THRESHOLD:
            {
                // Global punishment spam threshold (Condemnation / Unyielding Pain cast when no target nearby):
                // In small groups (mechanicParticipants < 4), suppress punishment bolts for idle corners.
                // 0 indicates punishment disabled for unengaged bosses.
                if (ctx.mechanicParticipants < 4)
                    return 0; // Suppress global punishment wipe
                return authoredValue;
            }
            default:
                return IEncounterAdapter::ResolveMechanic(mechanicId, type, authoredValue, ctx);
        }
    }
};

std::shared_ptr<IEncounterAdapter> CreateFourHorsemenAdapter()
{
    return std::make_shared<FourHorsemenAdapter>();
}
