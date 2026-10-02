/*
 * CoA Universal Content Scaling
 * TwinEmperorsAdapter: Adapts proximity heal distance and spell immunities for small groups.
 * Map: 531 (Temple of Ahn'Qiraj), Encounter: DATA_TWIN_EMPERORS (7)
 */

#include "AdaptiveEncounterAPI.h"

class TwinEmperorsAdapter : public IEncounterAdapter
{
public:
    [[nodiscard]] uint32 GetMapId() const override { return 531; }
    [[nodiscard]] uint32 GetEncounterId() const override { return 7; } // DATA_TWIN_EMPERORS
    [[nodiscard]] std::string_view GetName() const override { return "Twin Emperors"; }

    [[nodiscard]] uint32 ResolveMechanic(uint32 mechanicId, EncounterMechanicType type,
                                         uint32 authoredValue, EncounterContext const& ctx) const override
    {
        // Full group parity
        if (ctx.mechanicParticipants >= ctx.intendedPlayers && ctx.challengeSize == 0)
            return authoredValue;

        switch (type)
        {
            case EncounterMechanicType::PROXIMITY_DISTANCE:
            {
                // SPELL_HEAL_BROTHER proximity distance (authored 60 yards):
                // For solo (mechanicParticipants == 1), the bosses will inevitably stack on the player.
                // Disabling proximity heal (distance 0 yards) prevents infinite heal deadlock.
                // For 2 players (e.g. 2 tanks), keep distance lower or standard.
                if (ctx.isMechanicSolo)
                    return 0; // Effectively disables the heal check!
                if (ctx.mechanicParticipants <= 2)
                    return 20; // Tight window: must separate by 20y instead of 60y
                return authoredValue;
            }
            case EncounterMechanicType::REQUIRED_PLAYERS:
            {
                // Number of separate target positions / tanks required
                if (ctx.isMechanicSolo)
                    return 1;
                return std::min<uint32>(authoredValue, ctx.mechanicParticipants);
            }
            case EncounterMechanicType::ADD_COUNT:
            {
                // Bug spawns / mutate bug count
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

std::shared_ptr<IEncounterAdapter> CreateTwinEmperorsAdapter()
{
    return std::make_shared<TwinEmperorsAdapter>();
}
