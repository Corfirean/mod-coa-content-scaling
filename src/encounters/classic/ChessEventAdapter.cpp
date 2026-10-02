/*
 * CoA Universal Content Scaling
 * ChessEventAdapter: Karazhan Chess Event.
 * Map: 532 (Karazhan), Encounter: DATA_CHESS_EVENT (9)
 * Feasibility finding: Source audit confirms chess AI handles unpossessed pieces automatically.
 * Full parity and safe pass-through adapter.
 */

#include "AdaptiveEncounterAPI.h"

class ChessEventAdapter : public IEncounterAdapter
{
public:
    [[nodiscard]] uint32 GetMapId() const override { return 532; }
    [[nodiscard]] uint32 GetEncounterId() const override { return 9; } // DATA_CHESS_EVENT
    [[nodiscard]] std::string_view GetName() const override { return "Chess Event"; }

    [[nodiscard]] uint32 ResolveMechanic(uint32 mechanicId, EncounterMechanicType type,
                                         uint32 authoredValue, EncounterContext const& ctx) const override
    {
        // Full group parity
        if (ctx.mechanicParticipants >= ctx.intendedPlayers && ctx.challengeSize == 0)
            return authoredValue;

        switch (type)
        {
            case EncounterMechanicType::TIMER_MS:
            {
                // Medivh cheat fire timer (authored ~30-40s):
                // In solo mode, slightly increase cheat intervals so player can reposition pieces.
                if (ctx.isMechanicSolo)
                    return static_cast<uint32>(authoredValue * 1.5f);
                return authoredValue;
            }
            default:
                return authoredValue;
        }
    }
};

std::shared_ptr<IEncounterAdapter> CreateChessEventAdapter()
{
    return std::make_shared<ChessEventAdapter>();
}
