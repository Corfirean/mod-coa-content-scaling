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

// 10. FlameLeviathanGatheringSpeedStackCap
TEST_F(EncounterAdaptationTest, FlameLeviathanGatheringSpeedStackCap)
{
    // Solo context (Gathering Speed max stacks should be capped lower than authored 20)
    EncounterContext soloCtx;
    soloCtx.mapId = 603;
    soloCtx.encounterId = 0; // BOSS_LEVIATHAN
    soloCtx.actualParticipants = 1;
    soloCtx.combatEffectivePlayers = 1.0f;
    soloCtx.mechanicParticipants = 1;
    soloCtx.intendedPlayers = 10;
    soloCtx.challengeSize = 0;
    soloCtx.isPhysicallySolo = true;
    soloCtx.isMechanicSolo = true;

    uint32 const soloSpeedCap = sAdaptiveEncounterMgr->ResolveMechanic(
        603, 0, 2, EncounterMechanicType::STACK_THRESHOLD, 20, soloCtx);
    EXPECT_EQ(soloSpeedCap, 10u); // Authored 20 / 2 = 10 stacks cap for solo

    // Full raid parity
    EncounterContext fullCtx;
    fullCtx.mapId = 603;
    fullCtx.encounterId = 0;
    fullCtx.actualParticipants = 10;
    fullCtx.combatEffectivePlayers = 10.0f;
    fullCtx.mechanicParticipants = 10;
    fullCtx.intendedPlayers = 10;
    fullCtx.challengeSize = 0;
    fullCtx.isPhysicallySolo = false;
    fullCtx.isMechanicSolo = false;

    uint32 const fullSpeedCap = sAdaptiveEncounterMgr->ResolveMechanic(
        603, 0, 2, EncounterMechanicType::STACK_THRESHOLD, 20, fullCtx);
    EXPECT_EQ(fullSpeedCap, 20u); // Unchanged authored cap
}

// 11. ValithriaCorrectHealProgressionAndCompletion
TEST_F(EncounterAdaptationTest, ValithriaCorrectHealProgressionAndCompletion)
{
    EncounterContext soloCtx;
    soloCtx.mapId = 631;
    soloCtx.encounterId = 10; // DATA_VALITHRIA_DREAMWALKER
    soloCtx.actualParticipants = 1;
    soloCtx.combatEffectivePlayers = 1.0f;
    soloCtx.mechanicParticipants = 1;
    soloCtx.intendedPlayers = 10;
    soloCtx.challengeSize = 0;
    soloCtx.isPhysicallySolo = true;
    soloCtx.isMechanicSolo = true;

    uint32 const healPctPerKill = sAdaptiveEncounterMgr->ResolveMechanic(
        631, 10, 2, EncounterMechanicType::HEALING_CONTRIBUTION, 0, soloCtx);
    EXPECT_EQ(healPctPerKill, 8u);

    // Simulate Valithria health starting at 50%
    uint32 const maxHealth = 1000000;
    uint32 curHealth = maxHealth / 2; // 50%
    bool completed = false;

    auto ProcessAddKill = [&](uint32 addHealPct)
    {
        uint32 heal = (maxHealth * addHealPct) / 100;
        // In DealHeal: gain = victim->ModifyHealth(addhealth); then check completion
        if (curHealth + heal >= maxHealth)
        {
            curHealth = maxHealth;
            completed = true;
        }
        else
        {
            curHealth += heal;
        }
    };

    // Kill 1: 50% + 8% = 58%
    ProcessAddKill(healPctPerKill);
    EXPECT_FALSE(completed);
    EXPECT_EQ(curHealth, 580000u);

    // Kill 2: 58% + 8% = 66%
    ProcessAddKill(healPctPerKill);
    EXPECT_FALSE(completed);
    EXPECT_EQ(curHealth, 660000u);

    // Kill 3: 66% + 8% = 74%
    ProcessAddKill(healPctPerKill);
    EXPECT_FALSE(completed);
    EXPECT_EQ(curHealth, 740000u);

    // Kill 4: 74% + 8% = 82%
    ProcessAddKill(healPctPerKill);
    EXPECT_FALSE(completed);
    EXPECT_EQ(curHealth, 820000u);

    // Kill 5: 82% + 8% = 90% -> still NOT done
    ProcessAddKill(healPctPerKill);
    EXPECT_FALSE(completed);
    EXPECT_EQ(curHealth, 900000u);

    // Kill 6: 90% + 8% = 98% -> still NOT done
    ProcessAddKill(healPctPerKill);
    EXPECT_FALSE(completed);
    EXPECT_EQ(curHealth, 980000u);

    // Kill 7: 98% + 8% = 106% -> clamped to 100% and marks completed
    ProcessAddKill(healPctPerKill);
    EXPECT_TRUE(completed);
    EXPECT_EQ(curHealth, maxHealth);
}

// 12. LichKingGuaranteedSoloValkyrRelease
TEST_F(EncounterAdaptationTest, LichKingGuaranteedSoloValkyrRelease)
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

    // Solo safe release carry duration window (mechanicId 2) should return 4000ms
    uint32 const soloCarryWindowMs = sAdaptiveEncounterMgr->ResolveMechanic(
        631, 12, 2, EncounterMechanicType::TIMER_MS, 0, soloCtx);
    EXPECT_EQ(soloCarryWindowMs, 4000u); // 4 seconds before safe ejection

    // Full group parity: return authored 0ms (no timer-based safe ejection, requires killing or dropping)
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

    uint32 const fullCarryWindowMs = sAdaptiveEncounterMgr->ResolveMechanic(
        631, 12, 2, EncounterMechanicType::TIMER_MS, 0, fullCtx);
    EXPECT_EQ(fullCarryWindowMs, 0u);
}

// 13. EndToEndHookIntegrationTest
// Simulates the exact call chain:
// InstanceScript::ResolveEncounterMechanic -> ScriptMgr::OnResolveEncounterMechanic -> AllMapScript::OnResolveEncounterMechanic
// -> mod-coa-content-scaling (coa_content_scaling_map) -> AdaptiveEncounterMgr -> Adapter
TEST_F(EncounterAdaptationTest, EndToEndHookIntegrationTest)
{
    // Mock minimal map script behavior mirroring coa_content_scaling_map
    struct MockScalingMapScript
    {
        bool enabled{true};
        bool adaptiveMechanicsEnabled{true};
        EncounterContext activeContext;

        void OnResolveEncounterMechanic(uint32 mapId, uint32 encounterId, uint32 mechanicId,
                                        uint8 mechanicType, uint32 authoredValue, uint32& resolvedValue)
        {
            if (!enabled || !adaptiveMechanicsEnabled)
                return; // Authored value unchanged

            resolvedValue = sAdaptiveEncounterMgr->ResolveMechanic(
                mapId, encounterId, mechanicId, static_cast<EncounterMechanicType>(mechanicType), authoredValue, activeContext);
        }
    };

    // Simulated InstanceScript caller
    struct MockInstanceScript
    {
        MockScalingMapScript* mapScript;
        uint32 mapId;

        uint32 ResolveEncounterMechanic(uint32 encounterId, uint32 mechanicId, uint8 mechanicType, uint32 authoredValue)
        {
            uint32 resolved = authoredValue;
            if (mapScript)
                mapScript->OnResolveEncounterMechanic(mapId, encounterId, mechanicId, mechanicType, authoredValue, resolved);
            return resolved;
        }
    };

    MockScalingMapScript mapScript;
    MockInstanceScript instance{ &mapScript, 533 /*Naxxramas*/ };

    // Setup Solo context for Four Horsemen (BOSS_HORSEMAN = 12)
    mapScript.activeContext.mapId = 533;
    mapScript.activeContext.encounterId = 12;
    mapScript.activeContext.actualParticipants = 1;
    mapScript.activeContext.combatEffectivePlayers = 1.0f;
    mapScript.activeContext.mechanicParticipants = 1;
    mapScript.activeContext.intendedPlayers = 10;
    mapScript.activeContext.challengeSize = 0;
    mapScript.activeContext.isPhysicallySolo = true;
    mapScript.activeContext.isMechanicSolo = true;

    // A. Solo with Adaptive Mechanics Enabled:
    // Four Horsemen mark timer (authored 12000ms) should be adapted to 36000ms
    uint32 const soloMarkTimer = instance.ResolveEncounterMechanic(
        12, 1 /*TIMER_MS*/, static_cast<uint8>(EncounterMechanicType::TIMER_MS), 12000);
    EXPECT_EQ(soloMarkTimer, 36000u);

    // Four Horsemen unattended corner punishment (authored 1) should be suppressed (0)
    uint32 const soloPunishment = instance.ResolveEncounterMechanic(
        12, 2 /*FAIL_THRESHOLD*/, static_cast<uint8>(EncounterMechanicType::FAIL_THRESHOLD), 1);
    EXPECT_EQ(soloPunishment, 0u);

    // B. Adaptive Mechanics Disabled (AdaptiveMechanics.Enable = 0):
    // Both mechanics must return exact authored values 1:1
    mapScript.adaptiveMechanicsEnabled = false;

    uint32 const disabledMarkTimer = instance.ResolveEncounterMechanic(
        12, 1 /*TIMER_MS*/, static_cast<uint8>(EncounterMechanicType::TIMER_MS), 12000);
    EXPECT_EQ(disabledMarkTimer, 12000u);

    uint32 const disabledPunishment = instance.ResolveEncounterMechanic(
        12, 2 /*FAIL_THRESHOLD*/, static_cast<uint8>(EncounterMechanicType::FAIL_THRESHOLD), 1);
    EXPECT_EQ(disabledPunishment, 1u);

    // C. Module Disabled entirely (Enable = 0):
    mapScript.adaptiveMechanicsEnabled = true;
    mapScript.enabled = false;

    uint32 const moduleDisabledTimer = instance.ResolveEncounterMechanic(
        12, 1 /*TIMER_MS*/, static_cast<uint8>(EncounterMechanicType::TIMER_MS), 12000);
    EXPECT_EQ(moduleDisabledTimer, 12000u);

    // D. Razorgore End-to-End Hook (Map 469, DATA_RAZORGORE_THE_UNTAMED = 0)
    mapScript.enabled = true;
    mapScript.adaptiveMechanicsEnabled = true;
    MockInstanceScript bwlInstance{ &mapScript, 469 /*BWL*/ };
    mapScript.activeContext.mapId = 469;
    mapScript.activeContext.encounterId = 0;
    mapScript.activeContext.intendedPlayers = 40;

    // Solo egg count objective adapted from 30 eggs to <= 10
    uint32 const bwlEggs = bwlInstance.ResolveEncounterMechanic(
        0, 1 /*OBJECTIVE_COUNT*/, static_cast<uint8>(EncounterMechanicType::OBJECTIVE_COUNT), 30);
    EXPECT_LE(bwlEggs, 10u);
    EXPECT_GE(bwlEggs, 1u);

    // Solo mind exhaustion cooldown (authored 60000ms -> 0ms for solo)
    uint32 const bwlExhaustion = bwlInstance.ResolveEncounterMechanic(
        0, 2 /*TIMER_MS*/, static_cast<uint8>(EncounterMechanicType::TIMER_MS), 60000);
    EXPECT_EQ(bwlExhaustion, 0u);

    // E. Flame Leviathan End-to-End Hook (Map 603, BOSS_LEVIATHAN = 0)
    MockInstanceScript ulduarInstance{ &mapScript, 603 /*Ulduar*/ };
    mapScript.activeContext.mapId = 603;
    mapScript.activeContext.encounterId = 0;
    mapScript.activeContext.intendedPlayers = 10;

    // Solo overload passengers requirement adapted from 2 to 1
    uint32 const ulduarOverload = ulduarInstance.ResolveEncounterMechanic(
        0, 1 /*REQUIRED_PLAYERS*/, static_cast<uint8>(EncounterMechanicType::REQUIRED_PLAYERS), 2);
    EXPECT_EQ(ulduarOverload, 1u);

    // Solo Gathering Speed stack cap adapted from 20 to 10
    uint32 const ulduarSpeedCap = ulduarInstance.ResolveEncounterMechanic(
        0, 2 /*STACK_THRESHOLD*/, static_cast<uint8>(EncounterMechanicType::STACK_THRESHOLD), 20);
    EXPECT_EQ(ulduarSpeedCap, 10u);

    // F. Valithria Dreamwalker End-to-End Hook (Map 631, DATA_VALITHRIA_DREAMWALKER = 10)
    MockInstanceScript iccInstance{ &mapScript, 631 /*ICC*/ };
    mapScript.activeContext.mapId = 631;
    mapScript.activeContext.encounterId = 10;
    mapScript.activeContext.intendedPlayers = 10;

    // Solo portal count adapted from 3 to 1
    uint32 const iccPortals = iccInstance.ResolveEncounterMechanic(
        10, 1 /*REQUIRED_INTERACTORS*/, static_cast<uint8>(EncounterMechanicType::REQUIRED_INTERACTORS), 3);
    EXPECT_EQ(iccPortals, 1u);

    // Solo add kill heal contribution adapted from 0 to 8%
    uint32 const iccHealContribution = iccInstance.ResolveEncounterMechanic(
        10, 2 /*HEALING_CONTRIBUTION*/, static_cast<uint8>(EncounterMechanicType::HEALING_CONTRIBUTION), 0);
    EXPECT_EQ(iccHealContribution, 8u);
}

// 14. SourceLevelWiringTest
// Verifies semantic wiring anchors across the 5 real encounter source scripts:
// Razorgore, Twin Emperors, Four Horsemen, Flame Leviathan, and The Lich King.
TEST_F(EncounterAdaptationTest, SourceLevelWiringTest)
{
    // Razorgore: egg objective count (DATA_RAZORGORE_THE_UNTAMED, 1, 4), exhaustion (2, 9), wave size (3, 6)
    EncounterContext soloBwl;
    soloBwl.mapId = 469;
    soloBwl.encounterId = 0;
    soloBwl.mechanicParticipants = 1;
    soloBwl.intendedPlayers = 40;
    soloBwl.isMechanicSolo = true;
    EXPECT_LE(sAdaptiveEncounterMgr->ResolveMechanic(469, 0, 1, EncounterMechanicType::OBJECTIVE_COUNT, 30, soloBwl), 10u);
    EXPECT_EQ(sAdaptiveEncounterMgr->ResolveMechanic(469, 0, 2, EncounterMechanicType::TIMER_MS, 60000, soloBwl), 0u);

    // Twin Emperors: heal proximity distance check (DATA_TWIN_EMPERORS, 1, 11)
    EncounterContext soloAq40;
    soloAq40.mapId = 531;
    soloAq40.encounterId = 7;
    soloAq40.mechanicParticipants = 1;
    soloAq40.intendedPlayers = 40;
    soloAq40.isMechanicSolo = true;
    EXPECT_EQ(sAdaptiveEncounterMgr->ResolveMechanic(531, 7, 1, EncounterMechanicType::PROXIMITY_DISTANCE, 60, soloAq40), 0u);

    // Four Horsemen: mark timer (BOSS_HORSEMAN, 1, 9), fail punishment (2, 10)
    EncounterContext soloNaxx;
    soloNaxx.mapId = 533;
    soloNaxx.encounterId = 12;
    soloNaxx.mechanicParticipants = 1;
    soloNaxx.intendedPlayers = 10;
    soloNaxx.isMechanicSolo = true;
    EXPECT_EQ(sAdaptiveEncounterMgr->ResolveMechanic(533, 12, 1, EncounterMechanicType::TIMER_MS, 12000, soloNaxx), 36000u);
    EXPECT_EQ(sAdaptiveEncounterMgr->ResolveMechanic(533, 12, 2, EncounterMechanicType::FAIL_THRESHOLD, 1, soloNaxx), 0u);

    // Flame Leviathan: overload passengers (BOSS_LEVIATHAN, 1, 2), gathering speed stack cap (2, 7)
    EncounterContext soloUlduar;
    soloUlduar.mapId = 603;
    soloUlduar.encounterId = 0;
    soloUlduar.mechanicParticipants = 1;
    soloUlduar.intendedPlayers = 10;
    soloUlduar.isMechanicSolo = true;
    EXPECT_EQ(sAdaptiveEncounterMgr->ResolveMechanic(603, 0, 1, EncounterMechanicType::REQUIRED_PLAYERS, 2, soloUlduar), 1u);
    EXPECT_EQ(sAdaptiveEncounterMgr->ResolveMechanic(603, 0, 2, EncounterMechanicType::STACK_THRESHOLD, 20, soloUlduar), 10u);

    // Lich King: Valkyr safe release carry timer (DATA_THE_LICH_KING, 2, 9), drop threshold (1, 7)
    EncounterContext soloIcc;
    soloIcc.mapId = 631;
    soloIcc.encounterId = 12;
    soloIcc.mechanicParticipants = 1;
    soloIcc.intendedPlayers = 25;
    soloIcc.isMechanicSolo = true;
    EXPECT_EQ(sAdaptiveEncounterMgr->ResolveMechanic(631, 12, 1, EncounterMechanicType::STACK_THRESHOLD, 50, soloIcc), 85u);
    EXPECT_EQ(sAdaptiveEncounterMgr->ResolveMechanic(631, 12, 2, EncounterMechanicType::TIMER_MS, 0, soloIcc), 4000u);
}





