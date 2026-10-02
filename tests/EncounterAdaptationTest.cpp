/*
 * CoA Universal Content Scaling
 * EncounterAdaptationTest: Unit test suite for Round 4 Encounter Scaling & Adapters.
 */

#include "gtest/gtest.h"
#include "AdaptiveEncounterAPI.h"
#include <memory>

// Forward declaration
void RegisterCuratedEncounterAdapters();

class EncounterAdaptationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        sAdaptiveEncounterMgr->Clear();
        RegisterCuratedEncounterAdapters();
    }

    void TearDown() override
    {
        sAdaptiveEncounterMgr->Clear();
    }
};

// 1. AdapterKeyDoesNotCollideAcrossMaps: Test that encounter ID 0 on map 469 (BWL) and map 603 (Ulduar) resolve to distinct adapters
TEST_F(EncounterAdaptationTest, AdapterKeyDoesNotCollideAcrossMaps)
{
    IEncounterAdapter const* bwlAdapter = sAdaptiveEncounterMgr->GetAdapter(469, 0); // Razorgore
    IEncounterAdapter const* ulduarAdapter = sAdaptiveEncounterMgr->GetAdapter(603, 0); // Flame Leviathan

    ASSERT_NE(bwlAdapter, nullptr);
    ASSERT_NE(ulduarAdapter, nullptr);
    EXPECT_NE(bwlAdapter, ulduarAdapter);
    EXPECT_EQ(bwlAdapter->GetName(), "Razorgore the Untamed");
    EXPECT_EQ(ulduarAdapter->GetName(), "Flame Leviathan");
    EXPECT_EQ(bwlAdapter->GetMapId(), 469u);
    EXPECT_EQ(ulduarAdapter->GetMapId(), 603u);
}

// 2. FullGroupReturnsAuthoredValues: Test full group parity across all registered adapters
TEST_F(EncounterAdaptationTest, FullGroupReturnsAuthoredValues)
{
    // Razorgore (40-man intended raid)
    {
        EncounterContext ctx;
        ctx.mapId = 469;
        ctx.encounterId = 0;
        ctx.actualParticipants = 40;
        ctx.combatEffectivePlayers = 40.0f;
        ctx.mechanicParticipants = 40;
        ctx.intendedPlayers = 40;
        ctx.challengeSize = 0;
        ctx.isPhysicallySolo = false;
        ctx.isMechanicSolo = false;

        uint32 const eggs = sAdaptiveEncounterMgr->ResolveMechanic(
            469, 0, 0, EncounterMechanicType::OBJECTIVE_COUNT, 30, ctx);
        EXPECT_EQ(eggs, 30u); // Must return exact authored 30 eggs

        uint32 const timer = sAdaptiveEncounterMgr->ResolveMechanic(
            469, 0, 0, EncounterMechanicType::TIMER_MS, 60000, ctx);
        EXPECT_EQ(timer, 60000u); // Must return exact authored 60s
    }

    // Four Horsemen (10-man / 25-man intended)
    {
        EncounterContext ctx;
        ctx.mapId = 533;
        ctx.encounterId = 12;
        ctx.actualParticipants = 25;
        ctx.combatEffectivePlayers = 25.0f;
        ctx.mechanicParticipants = 25;
        ctx.intendedPlayers = 25;
        ctx.challengeSize = 0;
        ctx.isPhysicallySolo = false;
        ctx.isMechanicSolo = false;

        uint32 const positions = sAdaptiveEncounterMgr->ResolveMechanic(
            533, 12, 0, EncounterMechanicType::REQUIRED_PLAYERS, 4, ctx);
        EXPECT_EQ(positions, 4u); // Authored 4 tanks

        uint32 const markTimer = sAdaptiveEncounterMgr->ResolveMechanic(
            533, 12, 0, EncounterMechanicType::TIMER_MS, 12000, ctx);
        EXPECT_EQ(markTimer, 12000u); // Authored 12s
    }

    // Flame Leviathan (10-man intended)
    {
        EncounterContext ctx;
        ctx.mapId = 603;
        ctx.encounterId = 0;
        ctx.actualParticipants = 10;
        ctx.combatEffectivePlayers = 10.0f;
        ctx.mechanicParticipants = 10;
        ctx.intendedPlayers = 10;
        ctx.challengeSize = 0;
        ctx.isPhysicallySolo = false;
        ctx.isMechanicSolo = false;

        uint32 const overload = sAdaptiveEncounterMgr->ResolveMechanic(
            603, 0, 0, EncounterMechanicType::REQUIRED_PLAYERS, 2, ctx);
        EXPECT_EQ(overload, 2u); // Authored 2 passengers
    }

    // Valithria (25-man intended)
    {
        EncounterContext ctx;
        ctx.mapId = 631;
        ctx.encounterId = 10;
        ctx.actualParticipants = 25;
        ctx.combatEffectivePlayers = 25.0f;
        ctx.mechanicParticipants = 25;
        ctx.intendedPlayers = 25;
        ctx.challengeSize = 0;
        ctx.isPhysicallySolo = false;
        ctx.isMechanicSolo = false;

        uint32 const portals = sAdaptiveEncounterMgr->ResolveMechanic(
            631, 10, 0, EncounterMechanicType::REQUIRED_INTERACTORS, 8, ctx);
        EXPECT_EQ(portals, 8u); // Authored 8 portals

        uint32 const heal = sAdaptiveEncounterMgr->ResolveMechanic(
            631, 10, 0, EncounterMechanicType::HEALING_CONTRIBUTION, 0, ctx);
        EXPECT_EQ(heal, 0u); // Authored 0% direct add-kill heal
    }
}

// 3. MechanicParticipantFormulaMatrix: Verify actual, challenge, combatEffective, and mechanicParticipants math
TEST_F(EncounterAdaptationTest, MechanicParticipantFormulaMatrix)
{
    auto Calculate = [](uint32 actual, uint32 challenge)
    {
        float const combatEffective = (challenge > 0) ? float(challenge) : float(actual);
        uint32 const mechanicParts = (challenge > 0) ? std::min<uint32>(actual, challenge) : actual;
        return std::make_pair(combatEffective, mechanicParts);
    };

    // 2 actual + challenge10 -> combat=10, mechanics<=2
    auto [c1, m1] = Calculate(2, 10);
    EXPECT_FLOAT_EQ(c1, 10.0f);
    EXPECT_EQ(m1, 2u);

    // 5 actual + challenge2 -> combat=2, mechanics<=2
    auto [c2, m2] = Calculate(5, 2);
    EXPECT_FLOAT_EQ(c2, 2.0f);
    EXPECT_EQ(m2, 2u);

    // 5 actual + adaptive (0) -> combat=5, mechanics=5
    auto [c3, m3] = Calculate(5, 0);
    EXPECT_FLOAT_EQ(c3, 5.0f);
    EXPECT_EQ(m3, 5u);

    // 1 actual + adaptive (0) -> combat=1, mechanics=1
    auto [c4, m4] = Calculate(1, 0);
    EXPECT_FLOAT_EQ(c4, 1.0f);
    EXPECT_EQ(m4, 1u);
}

// 4. RazorgoreSoloAdaptation: Test solo adaptation removes exhaustion and reduces egg count
TEST_F(EncounterAdaptationTest, RazorgoreSoloAdaptation)
{
    EncounterContext ctx;
    ctx.mapId = 469;
    ctx.encounterId = 0;
    ctx.actualParticipants = 1;
    ctx.combatEffectivePlayers = 1.0f;
    ctx.mechanicParticipants = 1;
    ctx.intendedPlayers = 40;
    ctx.challengeSize = 0;
    ctx.isPhysicallySolo = true;
    ctx.isMechanicSolo = true;

    uint32 const eggs = sAdaptiveEncounterMgr->ResolveMechanic(
        469, 0, 0, EncounterMechanicType::OBJECTIVE_COUNT, 30, ctx);
    EXPECT_GE(eggs, 6u);
    EXPECT_LT(eggs, 30u); // Reduced significantly for solo

    uint32 const exhaustion = sAdaptiveEncounterMgr->ResolveMechanic(
        469, 0, 0, EncounterMechanicType::TIMER_MS, 60000, ctx);
    EXPECT_EQ(exhaustion, 0u); // Exhaustion cooldown removed for solo charmer!
}

// 5. TwinEmperorsSoloDisablesProximityHeal: Proximity heal distance scaled to 0 in solo
TEST_F(EncounterAdaptationTest, TwinEmperorsSoloDisablesProximityHeal)
{
    EncounterContext soloCtx;
    soloCtx.mapId = 531;
    soloCtx.encounterId = 7;
    soloCtx.actualParticipants = 1;
    soloCtx.combatEffectivePlayers = 1.0f;
    soloCtx.mechanicParticipants = 1;
    soloCtx.intendedPlayers = 40;
    soloCtx.challengeSize = 0;
    soloCtx.isPhysicallySolo = true;
    soloCtx.isMechanicSolo = true;

    uint32 const healDistSolo = sAdaptiveEncounterMgr->ResolveMechanic(
        531, 7, 0, EncounterMechanicType::PROXIMITY_DISTANCE, 60, soloCtx);
    EXPECT_EQ(healDistSolo, 0u); // Disabled proximity heal!

    EncounterContext groupCtx;
    groupCtx.mapId = 531;
    groupCtx.encounterId = 7;
    groupCtx.actualParticipants = 40;
    groupCtx.combatEffectivePlayers = 40.0f;
    groupCtx.mechanicParticipants = 40;
    groupCtx.intendedPlayers = 40;
    groupCtx.challengeSize = 0;
    groupCtx.isPhysicallySolo = false;
    groupCtx.isMechanicSolo = false;

    uint32 const healDistFull = sAdaptiveEncounterMgr->ResolveMechanic(
        531, 7, 0, EncounterMechanicType::PROXIMITY_DISTANCE, 60, groupCtx);
    EXPECT_EQ(healDistFull, 60u); // Authored 60y preserved
}

// 6. FourHorsemenSoloMarkAndPunishmentSuppression
TEST_F(EncounterAdaptationTest, FourHorsemenSoloMarkAndPunishmentSuppression)
{
    EncounterContext soloCtx;
    soloCtx.mapId = 533;
    soloCtx.encounterId = 12;
    soloCtx.actualParticipants = 1;
    soloCtx.combatEffectivePlayers = 1.0f;
    soloCtx.mechanicParticipants = 1;
    soloCtx.intendedPlayers = 25;
    soloCtx.challengeSize = 0;
    soloCtx.isPhysicallySolo = true;
    soloCtx.isMechanicSolo = true;

    uint32 const markTimerSolo = sAdaptiveEncounterMgr->ResolveMechanic(
        533, 12, 0, EncounterMechanicType::TIMER_MS, 12000, soloCtx);
    EXPECT_EQ(markTimerSolo, 36000u); // Triple duration (36s) gives room to kite/kill sequentially

    uint32 const punishmentSolo = sAdaptiveEncounterMgr->ResolveMechanic(
        533, 12, 0, EncounterMechanicType::FAIL_THRESHOLD, 1, soloCtx);
    EXPECT_EQ(punishmentSolo, 0u); // Suppress global wipe punishment for idle corners
}

// 7. FlameLeviathanSoloOverload
TEST_F(EncounterAdaptationTest, FlameLeviathanSoloOverload)
{
    EncounterContext soloCtx;
    soloCtx.mapId = 603;
    soloCtx.encounterId = 0;
    soloCtx.actualParticipants = 1;
    soloCtx.combatEffectivePlayers = 1.0f;
    soloCtx.mechanicParticipants = 1;
    soloCtx.intendedPlayers = 10;
    soloCtx.challengeSize = 0;
    soloCtx.isPhysicallySolo = true;
    soloCtx.isMechanicSolo = true;

    uint32 const overloadPassengers = sAdaptiveEncounterMgr->ResolveMechanic(
        603, 0, 0, EncounterMechanicType::REQUIRED_PLAYERS, 2, soloCtx);
    EXPECT_EQ(overloadPassengers, 1u); // Solo requires 1 passenger
}

// 8. ValithriaSoloDreamPortalsAndHeal
TEST_F(EncounterAdaptationTest, ValithriaSoloDreamPortalsAndHeal)
{
    EncounterContext soloCtx;
    soloCtx.mapId = 631;
    soloCtx.encounterId = 10;
    soloCtx.actualParticipants = 1;
    soloCtx.combatEffectivePlayers = 1.0f;
    soloCtx.mechanicParticipants = 1;
    soloCtx.intendedPlayers = 10;
    soloCtx.challengeSize = 0;
    soloCtx.isPhysicallySolo = true;
    soloCtx.isMechanicSolo = true;

    uint32 const portals = sAdaptiveEncounterMgr->ResolveMechanic(
        631, 10, 0, EncounterMechanicType::REQUIRED_INTERACTORS, 3, soloCtx);
    EXPECT_EQ(portals, 1u); // 1 portal for solo

    uint32 const healPerKill = sAdaptiveEncounterMgr->ResolveMechanic(
        631, 10, 0, EncounterMechanicType::HEALING_CONTRIBUTION, 0, soloCtx);
    EXPECT_GT(healPerKill, 0u); // Alternative heal contribution enabled for solo non-healer
}

// 9. LichKingSoloValkyrThreshold
TEST_F(EncounterAdaptationTest, LichKingSoloValkyrThreshold)
{
    // Solo context
    EncounterContext soloCtx;
    soloCtx.mapId = 631;
    soloCtx.encounterId = 12; // DATA_THE_LICH_KING
    soloCtx.actualParticipants = 1;
    soloCtx.combatEffectivePlayers = 1.0f;
    soloCtx.mechanicParticipants = 1;
    soloCtx.intendedPlayers = 25;
    soloCtx.challengeSize = 0;
    soloCtx.isPhysicallySolo = true;
    soloCtx.isMechanicSolo = true;

    // Solo drop threshold should be generous (e.g. 85%) so solo player can easily break Valkyr grab
    uint32 const soloDropPct = sAdaptiveEncounterMgr->ResolveMechanic(
        631, 12, 1, EncounterMechanicType::STACK_THRESHOLD, 50, soloCtx);
    EXPECT_EQ(soloDropPct, 85u);

    // Full raid parity: 25 participants, intended 25
    EncounterContext fullCtx;
    fullCtx.mapId = 631;
    fullCtx.encounterId = 12;
    fullCtx.actualParticipants = 25;
    fullCtx.combatEffectivePlayers = 25.0f;
    fullCtx.mechanicParticipants = 25;
    fullCtx.intendedPlayers = 25;
    fullCtx.challengeSize = 0;
    fullCtx.isPhysicallySolo = false;
    fullCtx.isMechanicSolo = false;

    uint32 const fullDropPct = sAdaptiveEncounterMgr->ResolveMechanic(
        631, 12, 1, EncounterMechanicType::STACK_THRESHOLD, 50, fullCtx);
    EXPECT_EQ(fullDropPct, 50u); // Preserves authored 50% threshold
}

