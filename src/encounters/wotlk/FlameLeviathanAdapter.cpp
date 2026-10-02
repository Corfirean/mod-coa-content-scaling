/*
 * CoA Universal Content Scaling
 * FlameLeviathanAdapter: Adapts vehicle speed gathering, overload mechanics, and turret demands.
 * Map: 603 (Ulduar), Encounter: BOSS_LEVIATHAN (0)
 */

#include "AdaptiveEncounterAPI.h"

class FlameLeviathanAdapter : public IEncounterAdapter
{
public:
    [[nodiscard]] uint32 GetMapId() const override { return 603; }
    [[nodiscard]] uint32 GetEncounterId() const override { return 0; } // BOSS_LEVIATHAN
    [[nodiscard]] std::string_view GetName() const override { return "Flame Leviathan"; }

    [[nodiscard]] uint32 ResolveMechanic(uint32 mechanicId, EncounterMechanicType type,
                                         uint32 authoredValue, EncounterContext const& ctx) const override
    {
        // Full group parity
        if (ctx.mechanicParticipants >= ctx.intendedPlayers && ctx.challengeSize == 0)
            return authoredValue;

        switch (type)
        {
            case EncounterMechanicType::REQUIRED_INTERACTORS:
            case EncounterMechanicType::REQUIRED_PLAYERS:
            {
                // Number of passengers launched onto Leviathan's back for overload (authored: 2)
                // In solo mode, 1 passenger can overload, or overload is triggered via vehicle ability.
                if (ctx.isMechanicSolo)
                    return 1;
                return std::min<uint32>(authoredValue, ctx.mechanicParticipants);
            }
            case EncounterMechanicType::STACK_THRESHOLD:
            {
                // Gathering Speed stack rate / max stacks (authored: 20 stacks)
                // Slow down speed stack buildup so a solo vehicle can kite without getting caught instantly.
                if (ctx.isMechanicSolo)
                    return std::max<uint32>(1, authoredValue / 2);
                return authoredValue;
            }
            case EncounterMechanicType::TIMER_MS:
            {
                // Pursued target switch timer (authored: 31000ms)
                // In small groups, allow longer pursuit or cooldowns between speed bursts.
                if (ctx.isMechanicSolo)
                    return static_cast<uint32>(authoredValue * 1.5f);
                return authoredValue;
            }
            default:
                return IEncounterAdapter::ResolveMechanic(mechanicId, type, authoredValue, ctx);
        }
    }
};

std::shared_ptr<IEncounterAdapter> CreateFlameLeviathanAdapter()
{
    return std::make_shared<FlameLeviathanAdapter>();
}
