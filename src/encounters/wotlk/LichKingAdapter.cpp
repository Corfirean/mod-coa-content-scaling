/*
 * CoA Universal Content Scaling
 * LichKingAdapter: Adapts Val'kyr Shadowguard grab mechanics, Defile scaling, and Harvest Soul demands.
 * Map: 631 (Icecrown Citadel), Encounter: DATA_THE_LICH_KING (12)
 */

#include "AdaptiveEncounterAPI.h"

class LichKingAdapter : public IEncounterAdapter
{
public:
    [[nodiscard]] uint32 GetMapId() const override { return 631; }
    [[nodiscard]] uint32 GetEncounterId() const override { return 12; } // DATA_THE_LICH_KING
    [[nodiscard]] std::string_view GetName() const override { return "The Lich King"; }

    [[nodiscard]] uint32 ResolveMechanic(uint32 mechanicId, EncounterMechanicType type,
                                         uint32 authoredValue, EncounterContext const& ctx) const override
    {
        // Full group parity
        if (ctx.mechanicParticipants >= ctx.intendedPlayers && ctx.challengeSize == 0)
            return authoredValue;

        switch (type)
        {
            case EncounterMechanicType::STACK_THRESHOLD:
            {
                // Valkyr release health threshold pct.
                // Authored: 50% in Heroic, 0% in Normal (in normal mode authored Valkyrs must be killed to 0 HP before reaching the edge).
                // In small groups / solo, drop threshold is set to 85% (heroic) or 75% (normal) so players can break free before being dropped off the platform.
                if (ctx.isMechanicSolo)
                    return 85;
                if (ctx.mechanicParticipants <= 3)
                    return 70;
                return authoredValue > 0 ? authoredValue : 50;
            }
            case EncounterMechanicType::REQUIRED_PLAYERS:
            {
                // Number of simultaneous non-tank targets required for mechanics (such as Valkyr grabs)
                if (ctx.isMechanicSolo)
                    return 1;
                return std::min<uint32>(authoredValue, ctx.mechanicParticipants);
            }
            case EncounterMechanicType::TIMER_MS:
            {
                // Cast/respawn timers for mechanics like Defile or Harvest Soul
                if (ctx.isMechanicSolo)
                    return static_cast<uint32>(authoredValue * 1.25f);
                return authoredValue;
            }
            default:
                return IEncounterAdapter::ResolveMechanic(mechanicId, type, authoredValue, ctx);
        }
    }
};

std::shared_ptr<IEncounterAdapter> CreateLichKingAdapter()
{
    return std::make_shared<LichKingAdapter>();
}
