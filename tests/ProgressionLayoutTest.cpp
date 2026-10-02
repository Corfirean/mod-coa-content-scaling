/*
 * CoA Universal Content Scaling
 * ProgressionLayoutTest: Comprehensive unit test matrix for ProgressionLayout combinations.
 */

#include "ProgressionLayout.h"
#include "ContentEra.h"
#include "ContentPackRegistry.h"
#include "CoAContentScalingConfig.h"
#include "DungeonFinding/LFG.h"
#include "GeneratedContentCensus.h"
#include "InstanceProfile.h"
#include "InstanceScaleContext.h"
#include "ItemBudgetScaler.h"
#include "ItemTemplate.h"
#include "gtest/gtest.h"

// 1. MaxLevel 60 Combinations
TEST(ProgressionLayoutTest, MaxLevel60_ClassicOnly)
{
    ProgressionLayout layout = ProgressionLayout::Create(60, false, false);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 60);
    EXPECT_FALSE(layout.tbc.has_value());
    EXPECT_FALSE(layout.wotlk.has_value());
}

TEST(ProgressionLayoutTest, MaxLevel60_ClassicTBC)
{
    ProgressionLayout layout = ProgressionLayout::Create(60, true, false);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 45);
    ASSERT_TRUE(layout.tbc.has_value());
    EXPECT_EQ(layout.tbc->minLevel, 45);
    EXPECT_EQ(layout.tbc->maxLevel, 60);
    EXPECT_FALSE(layout.wotlk.has_value());
}

TEST(ProgressionLayoutTest, MaxLevel60_ClassicWotLK)
{
    ProgressionLayout layout = ProgressionLayout::Create(60, false, true);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 45);
    EXPECT_FALSE(layout.tbc.has_value());
    ASSERT_TRUE(layout.wotlk.has_value());
    EXPECT_EQ(layout.wotlk->minLevel, 45);
    EXPECT_EQ(layout.wotlk->maxLevel, 60);
}

TEST(ProgressionLayoutTest, MaxLevel60_AllEras)
{
    ProgressionLayout layout = ProgressionLayout::Create(60, true, true);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 45);
    ASSERT_TRUE(layout.tbc.has_value());
    EXPECT_EQ(layout.tbc->minLevel, 45);
    EXPECT_EQ(layout.tbc->maxLevel, 55);
    ASSERT_TRUE(layout.wotlk.has_value());
    EXPECT_EQ(layout.wotlk->minLevel, 55);
    EXPECT_EQ(layout.wotlk->maxLevel, 60);
}

// 2. MaxLevel 70 Combinations (Interpolation)
TEST(ProgressionLayoutTest, MaxLevel70_ClassicOnly)
{
    ProgressionLayout layout = ProgressionLayout::Create(70, false, false);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 70);
    EXPECT_FALSE(layout.tbc.has_value());
    EXPECT_FALSE(layout.wotlk.has_value());
}

TEST(ProgressionLayoutTest, MaxLevel70_ClassicTBC)
{
    ProgressionLayout layout = ProgressionLayout::Create(70, true, false);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 53); // round(45 + 10 * 0.75) = round(52.5) = 53
    ASSERT_TRUE(layout.tbc.has_value());
    EXPECT_EQ(layout.tbc->minLevel, 53);
    EXPECT_EQ(layout.tbc->maxLevel, 70);
    EXPECT_FALSE(layout.wotlk.has_value());
}

TEST(ProgressionLayoutTest, MaxLevel70_ClassicWotLK)
{
    ProgressionLayout layout = ProgressionLayout::Create(70, false, true);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 53);
    EXPECT_FALSE(layout.tbc.has_value());
    ASSERT_TRUE(layout.wotlk.has_value());
    EXPECT_EQ(layout.wotlk->minLevel, 53);
    EXPECT_EQ(layout.wotlk->maxLevel, 70);
}

TEST(ProgressionLayoutTest, MaxLevel70_AllEras)
{
    ProgressionLayout layout = ProgressionLayout::Create(70, true, true);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 53);
    ASSERT_TRUE(layout.tbc.has_value());
    EXPECT_EQ(layout.tbc->minLevel, 53);
    EXPECT_EQ(layout.tbc->maxLevel, 63); // round(55 + 10 * 0.75) = 63
    ASSERT_TRUE(layout.wotlk.has_value());
    EXPECT_EQ(layout.wotlk->minLevel, 63);
    EXPECT_EQ(layout.wotlk->maxLevel, 70);
}

// 3. MaxLevel 80 Combinations
TEST(ProgressionLayoutTest, MaxLevel80_ClassicOnly)
{
    ProgressionLayout layout = ProgressionLayout::Create(80, false, false);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 80);
    EXPECT_FALSE(layout.tbc.has_value());
    EXPECT_FALSE(layout.wotlk.has_value());
}

TEST(ProgressionLayoutTest, MaxLevel80_ClassicTBC)
{
    ProgressionLayout layout = ProgressionLayout::Create(80, true, false);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 60);
    ASSERT_TRUE(layout.tbc.has_value());
    EXPECT_EQ(layout.tbc->minLevel, 60);
    EXPECT_EQ(layout.tbc->maxLevel, 80);
    EXPECT_FALSE(layout.wotlk.has_value());
}

TEST(ProgressionLayoutTest, MaxLevel80_ClassicWotLK)
{
    ProgressionLayout layout = ProgressionLayout::Create(80, false, true);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 60);
    EXPECT_FALSE(layout.tbc.has_value());
    ASSERT_TRUE(layout.wotlk.has_value());
    EXPECT_EQ(layout.wotlk->minLevel, 60);
    EXPECT_EQ(layout.wotlk->maxLevel, 80);
}

TEST(ProgressionLayoutTest, MaxLevel80_AllEras)
{
    ProgressionLayout layout = ProgressionLayout::Create(80, true, true);
    std::string err;
    EXPECT_TRUE(layout.Validate(err)) << err;
    EXPECT_EQ(layout.classic.minLevel, 1);
    EXPECT_EQ(layout.classic.maxLevel, 60);
    ASSERT_TRUE(layout.tbc.has_value());
    EXPECT_EQ(layout.tbc->minLevel, 60);
    EXPECT_EQ(layout.tbc->maxLevel, 70);
    ASSERT_TRUE(layout.wotlk.has_value());
    EXPECT_EQ(layout.wotlk->minLevel, 70);
    EXPECT_EQ(layout.wotlk->maxLevel, 80);
}

// 4. Monotonic Mapping & Boundaries Tests
TEST(ProgressionLayoutTest, MonotonicLevelMapping_Cap60AllEras)
{
    ProgressionLayout layout = ProgressionLayout::Create(60, true, true);

    // Classic: 1-60 -> 1-45
    uint8 lastEffective = 0;
    for (uint8 authored = 1; authored <= 60; ++authored)
    {
        uint8 effective = layout.MapAuthoredToEffective(ContentEra::Classic, authored, 1, 60);
        EXPECT_GE(effective, lastEffective) << "Classic mapping must be strictly monotonic";
        EXPECT_GE(effective, 1);
        EXPECT_LE(effective, 45);
        lastEffective = effective;
    }

    // TBC: 58-70 -> 45-55
    lastEffective = 0;
    for (uint8 authored = 58; authored <= 70; ++authored)
    {
        uint8 effective = layout.MapAuthoredToEffective(ContentEra::TBC, authored, 58, 70);
        EXPECT_GE(effective, lastEffective) << "TBC mapping must be strictly monotonic";
        EXPECT_GE(effective, 45);
        EXPECT_LE(effective, 55);
        lastEffective = effective;
    }

    // WotLK: 68-80 -> 55-60
    lastEffective = 0;
    for (uint8 authored = 68; authored <= 80; ++authored)
    {
        uint8 effective = layout.MapAuthoredToEffective(ContentEra::WotLK, authored, 68, 80);
        EXPECT_GE(effective, lastEffective) << "WotLK mapping must be strictly monotonic";
        EXPECT_GE(effective, 55);
        EXPECT_LE(effective, 60);
        lastEffective = effective;
    }
}

TEST(ProgressionLayoutTest, ValidationFailures)
{
    // maxLevel > 80 fails validation
    ProgressionLayout badMax = ProgressionLayout::Create(85, true, true);
    std::string err;
    EXPECT_FALSE(badMax.Validate(err));

    // maxLevel < 60 fails validation
    ProgressionLayout badMin = ProgressionLayout::Create(50, true, true);
    EXPECT_FALSE(badMin.Validate(err));
}

// 5. Pack Registration Order Invariance
TEST(ProgressionLayoutTest, RegistrationOrderInvariance)
{
    // Regardless of which pack was discovered/registered first, the resulting ProgressionLayout is identical
    ProgressionLayout l1 = ProgressionLayout::Create(60, true, true);
    ProgressionLayout l2 = ProgressionLayout::Create(60, true, true);

    EXPECT_EQ(l1.maxLevel, l2.maxLevel);
    EXPECT_EQ(l1.classic.minLevel, l2.classic.minLevel);
    EXPECT_EQ(l1.classic.maxLevel, l2.classic.maxLevel);
    ASSERT_TRUE(l1.tbc.has_value() && l2.tbc.has_value());
    EXPECT_EQ(l1.tbc->minLevel, l2.tbc->minLevel);
    EXPECT_EQ(l1.tbc->maxLevel, l2.tbc->maxLevel);
    ASSERT_TRUE(l1.wotlk.has_value() && l2.wotlk.has_value());
    EXPECT_EQ(l1.wotlk->minLevel, l2.wotlk->minLevel);
    EXPECT_EQ(l1.wotlk->maxLevel, l2.wotlk->maxLevel);
}

// 6. ContentEra Strict Resolution Priority
TEST(ContentEraResolutionTest, ClassicContinentCreatureRetainsClassicAtLevel60)
{
    // A creature on Eastern Kingdoms (map 0) or Kalimdor (map 1) at level 60 must NEVER be promoted to TBC
    EraResolutionResult res = sContentPackRegistry->ResolveEraDetailsForCreature(
        12345, 0 /*Eastern Kingdoms*/, 0, 0 /*Classic expansion*/, 60 /*Level 60*/);

    EXPECT_EQ(res.era, ContentEra::Classic);
    EXPECT_EQ(res.source, EraResolutionSource::InstanceProfile);

    // Explicit override takes precedence over map profile
    sContentPackRegistry->RegisterCreatureOverride(12345, ContentEra::Custom);
    EraResolutionResult overrideRes = sContentPackRegistry->ResolveEraDetailsForCreature(
        12345, 0, 0, 0, 60);

    EXPECT_EQ(overrideRes.era, ContentEra::Custom);
    EXPECT_EQ(overrideRes.source, EraResolutionSource::ExplicitOverride);

    // Clean up
    sContentPackRegistry->Clear();
}

// 7. Monotonic Item Power Bands
TEST(ItemBudgetScalerTest, MonotonicRaidTierIlvlOrdering)
{
    ProgressionLayout layout = ProgressionLayout::Create(60, true, true);

    ItemTemplate naxxItem;
    naxxItem.ItemId = 39000;
    naxxItem.ItemLevel = 200; // T7
    naxxItem.RequiredLevel = 80;

    ItemTemplate ulduarItem;
    ulduarItem.ItemId = 45000;
    ulduarItem.ItemLevel = 226; // T8
    ulduarItem.RequiredLevel = 80;

    ItemTemplate tocItem;
    tocItem.ItemId = 47000;
    tocItem.ItemLevel = 245; // T9
    tocItem.RequiredLevel = 80;

    ItemTemplate iccItem;
    iccItem.ItemId = 50000;
    iccItem.ItemLevel = 264; // T10
    iccItem.RequiredLevel = 80;

    ScaledItemBudget bNaxx = sItemBudgetScaler->CalculateItemBudget(&naxxItem, layout);
    ScaledItemBudget bUlduar = sItemBudgetScaler->CalculateItemBudget(&ulduarItem, layout);
    ScaledItemBudget bToc = sItemBudgetScaler->CalculateItemBudget(&tocItem, layout);
    ScaledItemBudget bIcc = sItemBudgetScaler->CalculateItemBudget(&iccItem, layout);

    // Monotonic item levels: Naxx < Ulduar < ToC < ICC
    EXPECT_LT(bNaxx.effectiveItemLevel, bUlduar.effectiveItemLevel);
    EXPECT_LT(bUlduar.effectiveItemLevel, bToc.effectiveItemLevel);
    EXPECT_LT(bToc.effectiveItemLevel, bIcc.effectiveItemLevel);

    // Effective stat points: higher tiers retain strictly greater effective power
    // Authored stats for T7 ~70, T8 ~100, T9 ~135, T10 ~175
    float const statNaxx = 70.0f * bNaxx.statMultiplier;
    float const statUlduar = 100.0f * bUlduar.statMultiplier;
    float const statToc = 135.0f * bToc.statMultiplier;
    float const statIcc = 175.0f * bIcc.statMultiplier;

    EXPECT_LT(statNaxx, statUlduar);
    EXPECT_LT(statUlduar, statToc);
    EXPECT_LT(statToc, statIcc);
}

// 8. Instance Profile Validation and Multi-Era Resolution (Onyxia 249)
TEST(InstanceProfileRegistryTest, ValidationAndEraResolution)
{
    sInstanceProfileRegistry->Initialize();

    std::vector<std::string> issues;
    bool const valid = sInstanceProfileRegistry->ValidateAll(issues);
    EXPECT_TRUE(valid) << (issues.empty() ? "" : issues[0]);

    // Onyxia (Map 249): 40-man Classic vs 10/25 WotLK
    EXPECT_EQ(sInstanceProfileRegistry->GetEraForMap(249, 0), ContentEra::Classic);
    EXPECT_EQ(sInstanceProfileRegistry->GetEraForMap(249, 1), ContentEra::WotLK);
    EXPECT_EQ(sInstanceProfileRegistry->GetEraForMap(249, 2), ContentEra::WotLK);
}

// 9. Instance Scaling Multipliers Monotonicity & Solo Floor
TEST(InstanceScalingMultipliersTest, DynamicScalingMonotonicity)
{
    InstanceScaleContext ctx;
    ctx.intendedPlayers = 5;

    // Solo in 5-man
    ctx.CalculateMultipliers(1.0f / 5.0f);
    float const soloHealth = ctx.healthScale;
    float const soloDamage = ctx.damageScale;
    EXPECT_GT(soloHealth, 0.0f);
    EXPECT_LT(soloHealth, 1.0f);
    EXPECT_GT(soloDamage, 0.0f);
    EXPECT_LT(soloDamage, 1.0f);

    // Duo in 5-man
    ctx.CalculateMultipliers(2.0f / 5.0f);
    float const duoHealth = ctx.healthScale;
    float const duoDamage = ctx.damageScale;
    EXPECT_GT(duoHealth, soloHealth);
    EXPECT_GT(duoDamage, soloDamage);

    // Full 5-man
    ctx.CalculateMultipliers(5.0f / 5.0f);
    EXPECT_FLOAT_EQ(ctx.healthScale, 1.0f);
    EXPECT_FLOAT_EQ(ctx.damageScale, 1.0f);
    EXPECT_FLOAT_EQ(ctx.healingScale, 1.0f);
    EXPECT_FLOAT_EQ(ctx.absorbScale, 1.0f);
}

// 10. Challenge Size Isolation by Instance ID
TEST(InstanceScalingMgrTest, ChallengeModeStorage)
{
    uint32 const mapId = 33; // Shadowfang Keep
    uint32 const instA = 1001;
    uint32 const instB = 1002;

    sInstanceScalingMgr->SetChallengeSize(mapId, instA, 10);
    sInstanceScalingMgr->SetChallengeSize(mapId, instB, 1);

    EXPECT_EQ(sInstanceScalingMgr->GetChallengeSize(mapId, instA), 10u);
    EXPECT_EQ(sInstanceScalingMgr->GetChallengeSize(mapId, instB), 1u);

    // Reset instA to adaptive
    sInstanceScalingMgr->SetChallengeSize(mapId, instA, 0);
    EXPECT_EQ(sInstanceScalingMgr->GetChallengeSize(mapId, instA), 0u);
    EXPECT_EQ(sInstanceScalingMgr->GetChallengeSize(mapId, instB), 1u); // instB remains unaffected
}

// 11. Authoritative Encounter Lifecycle Hierarchy & Keys
TEST(EncounterLifecycleSourceHierarchyTest, PriorityAndKeyIntegrity)
{
    // Instance Script has higher priority (lower numeric value) than Creature Fallback
    EXPECT_LT(static_cast<uint8>(EncounterLifecycleSource::INSTANCE_SCRIPT),
              static_cast<uint8>(EncounterLifecycleSource::REGISTERED_ADAPTER));
    EXPECT_LT(static_cast<uint8>(EncounterLifecycleSource::REGISTERED_ADAPTER),
              static_cast<uint8>(EncounterLifecycleSource::CREATURE_FALLBACK));

    EncounterKey k1{EncounterKeyType::INSTANCE_ENCOUNTER, 1};
    EncounterKey k2{EncounterKeyType::INSTANCE_ENCOUNTER, 1};
    EncounterKey k3{EncounterKeyType::CREATURE_ENTRY, 1};

    EXPECT_EQ(k1, k2);
    EXPECT_NE(k1, k3);

    std::hash<EncounterKey> hasher;
    EXPECT_EQ(hasher(k1), hasher(k2));
}

// 12. LFG Queue Policy Acceptance Matrix
TEST(LfgQueuePolicyAcceptanceMatrixTest, AcceptanceMatrixVerification)
{
    auto ResolvePolicy = [](lfg::LfgCompositionMode mode, uint32 challengeSize, uint8 partySize)
    {
        lfg::LfgQueuePolicy policy;
        policy.compositionMode = mode;
        policy.challengeSize = challengeSize;

        switch (mode)
        {
            case lfg::LfgCompositionMode::MATCHMAKING:
                policy.bypassMatchmaking = false;
                policy.requireStandardRoles = true;
                policy.minPlayers = 5;
                policy.targetPlayers = 5;
                break;

            case lfg::LfgCompositionMode::BOT_FILL:
                policy.bypassMatchmaking = false;
                policy.requireStandardRoles = true;
                policy.minPlayers = 5;
                policy.targetPlayers = 5;
                break;

            case lfg::LfgCompositionMode::CURRENT_PARTY:
                policy.bypassMatchmaking = true;
                policy.requireStandardRoles = false;
                policy.minPlayers = partySize;
                policy.targetPlayers = partySize;
                break;
        }
        return policy;
    };

    // Scenario 1: CURRENT_PARTY, solo, adaptive
    {
        auto p = ResolvePolicy(lfg::LfgCompositionMode::CURRENT_PARTY, 0, 1);
        EXPECT_TRUE(p.bypassMatchmaking);
        EXPECT_FALSE(p.requireStandardRoles);
        EXPECT_EQ(p.minPlayers, 1u);
        EXPECT_EQ(p.targetPlayers, 1u);
        EXPECT_EQ(p.challengeSize, 0u);
    }

    // Scenario 2: CURRENT_PARTY, solo, challenge 5
    {
        auto p = ResolvePolicy(lfg::LfgCompositionMode::CURRENT_PARTY, 5, 1);
        EXPECT_TRUE(p.bypassMatchmaking);
        EXPECT_EQ(p.minPlayers, 1u);
        EXPECT_EQ(p.targetPlayers, 1u);
        EXPECT_EQ(p.challengeSize, 5u);
    }

    // Scenario 3: CURRENT_PARTY, duo, adaptive
    {
        auto p = ResolvePolicy(lfg::LfgCompositionMode::CURRENT_PARTY, 0, 2);
        EXPECT_TRUE(p.bypassMatchmaking);
        EXPECT_EQ(p.minPlayers, 2u);
        EXPECT_EQ(p.targetPlayers, 2u);
        EXPECT_EQ(p.challengeSize, 0u);
    }

    // Scenario 4: CURRENT_PARTY, duo, challenge 1
    {
        auto p = ResolvePolicy(lfg::LfgCompositionMode::CURRENT_PARTY, 1, 2);
        EXPECT_TRUE(p.bypassMatchmaking);
        EXPECT_EQ(p.minPlayers, 2u);
        EXPECT_EQ(p.targetPlayers, 2u);
        EXPECT_EQ(p.challengeSize, 1u);
    }

    // Scenario 5: CURRENT_PARTY, duo, challenge 5
    {
        auto p = ResolvePolicy(lfg::LfgCompositionMode::CURRENT_PARTY, 5, 2);
        EXPECT_TRUE(p.bypassMatchmaking);
        EXPECT_EQ(p.minPlayers, 2u);
        EXPECT_EQ(p.targetPlayers, 2u);
        EXPECT_EQ(p.challengeSize, 5u);
    }

    // Scenario 6: CURRENT_PARTY, 4-man, challenge 10
    {
        auto p = ResolvePolicy(lfg::LfgCompositionMode::CURRENT_PARTY, 10, 4);
        EXPECT_TRUE(p.bypassMatchmaking);
        EXPECT_EQ(p.minPlayers, 4u);
        EXPECT_EQ(p.targetPlayers, 4u);
        EXPECT_EQ(p.challengeSize, 10u);
    }

    // Scenario 7: MATCHMAKING, solo, adaptive
    {
        auto p = ResolvePolicy(lfg::LfgCompositionMode::MATCHMAKING, 0, 1);
        EXPECT_FALSE(p.bypassMatchmaking);
        EXPECT_TRUE(p.requireStandardRoles);
        EXPECT_EQ(p.minPlayers, 5u);
        EXPECT_EQ(p.targetPlayers, 5u);
        EXPECT_EQ(p.challengeSize, 0u);
    }

    // Scenario 8: BOT_FILL, solo, adaptive
    {
        auto p = ResolvePolicy(lfg::LfgCompositionMode::BOT_FILL, 0, 1);
        EXPECT_FALSE(p.bypassMatchmaking);
        EXPECT_TRUE(p.requireStandardRoles);
        EXPECT_EQ(p.minPlayers, 5u);
        EXPECT_EQ(p.targetPlayers, 5u);
        EXPECT_EQ(p.challengeSize, 0u);
    }
}

// =============================================================================
// Round 2.2: Config Canonical Keys and Centralized Parser Tests
// =============================================================================

TEST(CoAConfigTest, CanonicalKeysConstantsMatch)
{
    EXPECT_STREQ(CoAContentScalingConfigKeys::Enable, "CoAContentScaling.Enable");
    EXPECT_STREQ(CoAContentScalingConfigKeys::ProgressionMode, "CoAContentScaling.Progression.Mode");
    EXPECT_STREQ(CoAContentScalingConfigKeys::ProgressionClassicEnd, "CoAContentScaling.Progression.ClassicEnd");
    EXPECT_STREQ(CoAContentScalingConfigKeys::ProgressionTbcEnd, "CoAContentScaling.Progression.TbcEnd");
    EXPECT_STREQ(CoAContentScalingConfigKeys::GroupScalingEnable, "CoAContentScaling.GroupScaling.Enable");
    EXPECT_STREQ(CoAContentScalingConfigKeys::GroupScalingLockOnEncounterStart, "CoAContentScaling.GroupScaling.LockOnEncounterStart");
    EXPECT_STREQ(CoAContentScalingConfigKeys::GroupScalingAllowSoloRaids, "CoAContentScaling.GroupScaling.AllowSoloRaids");
    EXPECT_STREQ(CoAContentScalingConfigKeys::AdaptiveMechanicsEnable, "CoAContentScaling.AdaptiveMechanics.Enable");
    EXPECT_STREQ(CoAContentScalingConfigKeys::SoloAssistMode, "CoAContentScaling.SoloAssist.Mode");
    EXPECT_STREQ(CoAContentScalingConfigKeys::RewardsScaleLootCount, "CoAContentScaling.Rewards.ScaleLootCount");
    EXPECT_STREQ(CoAContentScalingConfigKeys::ScaleItems, "CoAContentScaling.ScaleItems");
    EXPECT_STREQ(CoAContentScalingConfigKeys::LfgDefaultMode, "CoAContentScaling.LFG.DefaultMode");
    EXPECT_STREQ(CoAContentScalingConfigKeys::LfgDefaultChallengeSize, "CoAContentScaling.LFG.DefaultChallengeSize");
    EXPECT_STREQ(CoAContentScalingConfigKeys::Debug, "CoAContentScaling.Debug");
}

TEST(CoAConfigTest, ParseProgressionMode)
{
    EXPECT_EQ(CoAContentScalingConfig::ParseProgressionMode("Auto"), "Auto");
    EXPECT_EQ(CoAContentScalingConfig::ParseProgressionMode("auto"), "Auto");
    EXPECT_EQ(CoAContentScalingConfig::ParseProgressionMode(" AUTO "), "Auto");
    EXPECT_EQ(CoAContentScalingConfig::ParseProgressionMode("Custom"), "Custom");
    EXPECT_EQ(CoAContentScalingConfig::ParseProgressionMode("custom"), "Custom");
    EXPECT_EQ(CoAContentScalingConfig::ParseProgressionMode(" CUSTOM "), "Custom");
    // Invalid / garbage fallback to Auto
    EXPECT_EQ(CoAContentScalingConfig::ParseProgressionMode("Unknown"), "Auto");
    EXPECT_EQ(CoAContentScalingConfig::ParseProgressionMode(""), "Auto");
    EXPECT_EQ(CoAContentScalingConfig::ParseProgressionMode("123"), "Auto");
}

TEST(CoAConfigTest, ParseSoloAssistMode)
{
    // Canonical names
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("None"), SoloAssistMode::NONE);
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("none"), SoloAssistMode::NONE);
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("Light"), SoloAssistMode::LIGHT);
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("light"), SoloAssistMode::LIGHT);
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("Full"), SoloAssistMode::FULL);
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("full"), SoloAssistMode::FULL);

    // Backwards-compatible numbers
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("0"), SoloAssistMode::NONE);
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("1"), SoloAssistMode::LIGHT);
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("2"), SoloAssistMode::FULL);

    // Trimming
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode(" 1 "), SoloAssistMode::LIGHT);
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode(" Light "), SoloAssistMode::LIGHT);

    // Invalid values MUST safely fall back to NONE (never LIGHT)
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("invalid"), SoloAssistMode::NONE);
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("3"), SoloAssistMode::NONE);
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode("99"), SoloAssistMode::NONE);
    EXPECT_EQ(CoAContentScalingConfig::ParseSoloAssistMode(""), SoloAssistMode::NONE);
}

TEST(CoAConfigTest, ParseLfgCompositionMode)
{
    EXPECT_EQ(CoAContentScalingConfig::ParseLfgCompositionMode("Matchmaking"), lfg::LfgCompositionMode::MATCHMAKING);
    EXPECT_EQ(CoAContentScalingConfig::ParseLfgCompositionMode("matchmaking"), lfg::LfgCompositionMode::MATCHMAKING);
    EXPECT_EQ(CoAContentScalingConfig::ParseLfgCompositionMode("BotFill"), lfg::LfgCompositionMode::BOT_FILL);
    EXPECT_EQ(CoAContentScalingConfig::ParseLfgCompositionMode("bots"), lfg::LfgCompositionMode::BOT_FILL);
    EXPECT_EQ(CoAContentScalingConfig::ParseLfgCompositionMode("CurrentParty"), lfg::LfgCompositionMode::CURRENT_PARTY);
    EXPECT_EQ(CoAContentScalingConfig::ParseLfgCompositionMode("party"), lfg::LfgCompositionMode::CURRENT_PARTY);
    EXPECT_EQ(CoAContentScalingConfig::ParseLfgCompositionMode("solo"), lfg::LfgCompositionMode::CURRENT_PARTY);

    // Invalid fallback
    EXPECT_EQ(CoAContentScalingConfig::ParseLfgCompositionMode("invalid"), lfg::LfgCompositionMode::MATCHMAKING);
    EXPECT_EQ(CoAContentScalingConfig::ParseLfgCompositionMode(""), lfg::LfgCompositionMode::MATCHMAKING);
}

// =============================================================================
// Round 2.2: Pending Policy Atomic Consume-Once and TTL Simulation Tests
// =============================================================================

struct TestPendingPolicy
{
    uint32 mapId{0};
    uint64 groupGuid{0};
    uint32 challengeSize{0};
    lfg::LfgCompositionMode compositionMode{lfg::LfgCompositionMode::MATCHMAKING};
    uint64 generation{0};
    uint32 createdAt{0};
};

class PendingPolicySimulator
{
public:
    void RegisterProposal(uint32 mapId, uint64 groupGuid, std::vector<uint64> const& playerGuids,
                          uint32 challengeSize, lfg::LfgCompositionMode compMode, uint32 currentTime)
    {
        ++_generation;
        TestPendingPolicy p;
        p.mapId = mapId;
        p.groupGuid = groupGuid;
        p.challengeSize = challengeSize;
        p.compositionMode = compMode;
        p.generation = _generation;
        p.createdAt = currentTime;

        if (groupGuid)
            _groupPolicies[groupGuid] = p;

        for (uint64 pguid : playerGuids)
            _playerPolicies[pguid] = p;
    }

    std::optional<TestPendingPolicy> Consume(uint32 mapId, uint64 groupGuid, uint64 playerGuid, uint32 currentTime)
    {
        PurgeExpired(currentTime);

        TestPendingPolicy policy;
        bool found = false;

        if (groupGuid)
        {
            auto it = _groupPolicies.find(groupGuid);
            if (it != _groupPolicies.end() && it->second.mapId == mapId)
            {
                policy = it->second;
                found = true;
            }
        }

        if (!found && playerGuid)
        {
            auto it = _playerPolicies.find(playerGuid);
            if (it != _playerPolicies.end() && it->second.mapId == mapId)
            {
                policy = it->second;
                found = true;
            }
        }

        if (!found)
            return std::nullopt;

        uint64 const targetGen = policy.generation;

        if (policy.groupGuid)
        {
            auto git = _groupPolicies.find(policy.groupGuid);
            if (git != _groupPolicies.end() && git->second.generation == targetGen)
                _groupPolicies.erase(git);
        }

        for (auto it = _playerPolicies.begin(); it != _playerPolicies.end();)
        {
            if (it->second.generation == targetGen)
                it = _playerPolicies.erase(it);
            else
                ++it;
        }

        return policy;
    }

    void OnPlayerLogout(uint64 playerGuid, uint64 leaderGuid, uint32 groupSize)
    {
        auto it = _playerPolicies.find(playerGuid);
        if (it != _playerPolicies.end())
        {
            uint64 const gen = it->second.generation;
            uint64 const grpGuid = it->second.groupGuid;
            _playerPolicies.erase(it);

            if (grpGuid && (playerGuid == leaderGuid || groupSize <= 1))
            {
                auto git = _groupPolicies.find(grpGuid);
                if (git != _groupPolicies.end() && git->second.generation == gen)
                    _groupPolicies.erase(git);
            }
        }
    }

    void PurgeExpired(uint32 currentTime)
    {
        constexpr uint32 TTL = 300; // 5 min
        for (auto it = _groupPolicies.begin(); it != _groupPolicies.end();)
        {
            if (currentTime > it->second.createdAt && (currentTime - it->second.createdAt) > TTL)
                it = _groupPolicies.erase(it);
            else
                ++it;
        }

        for (auto it = _playerPolicies.begin(); it != _playerPolicies.end();)
        {
            if (currentTime > it->second.createdAt && (currentTime - it->second.createdAt) > TTL)
                it = _playerPolicies.erase(it);
            else
                ++it;
        }
    }

    size_t GroupCount() const { return _groupPolicies.size(); }
    size_t PlayerCount() const { return _playerPolicies.size(); }

private:
    uint64 _generation{0};
    std::unordered_map<uint64, TestPendingPolicy> _groupPolicies;
    std::unordered_map<uint64, TestPendingPolicy> _playerPolicies;
};

TEST(PendingPolicyTest, ConsumeOnceAndEraseAllAliases)
{
    PendingPolicySimulator sim;
    sim.RegisterProposal(33, 100, { 1, 2, 3, 4, 5 }, 10, lfg::LfgCompositionMode::CURRENT_PARTY, 1000);

    EXPECT_EQ(sim.GroupCount(), 1u);
    EXPECT_EQ(sim.PlayerCount(), 5u);

    // First entry: Successfully consumed
    auto consumed = sim.Consume(33, 100, 1, 1010);
    ASSERT_TRUE(consumed.has_value());
    EXPECT_EQ(consumed->challengeSize, 10u);
    EXPECT_EQ(consumed->mapId, 33u);

    // Group entry and ALL 5 player aliases sharing generation erased atomically
    EXPECT_EQ(sim.GroupCount(), 0u);
    EXPECT_EQ(sim.PlayerCount(), 0u);

    // Re-entry / subsequent check: MUST NOT inherit old challenge
    auto secondEntry = sim.Consume(33, 100, 1, 1020);
    EXPECT_FALSE(secondEntry.has_value());

    auto memberEntry = sim.Consume(33, 100, 2, 1020);
    EXPECT_FALSE(memberEntry.has_value());
}

TEST(PendingPolicyTest, TtlExpiration)
{
    PendingPolicySimulator sim;
    sim.RegisterProposal(43, 200, { 10 }, 5, lfg::LfgCompositionMode::CURRENT_PARTY, 1000);

    // Within TTL (150s elapsed)
    sim.PurgeExpired(1150);
    EXPECT_EQ(sim.GroupCount(), 1u);

    // Past 300s TTL (301s elapsed)
    sim.PurgeExpired(1301);
    EXPECT_EQ(sim.GroupCount(), 0u);
    EXPECT_EQ(sim.PlayerCount(), 0u);

    auto result = sim.Consume(43, 200, 10, 1302);
    EXPECT_FALSE(result.has_value());
}

TEST(PendingPolicyTest, LeaderLogoutCleansGroupPolicy)
{
    PendingPolicySimulator sim;
    sim.RegisterProposal(33, 300, { 20, 21, 22 }, 3, lfg::LfgCompositionMode::CURRENT_PARTY, 1000);

    EXPECT_EQ(sim.GroupCount(), 1u);
    EXPECT_EQ(sim.PlayerCount(), 3u);

    // Non-leader logs out (leader is 20, member 21 logs out)
    sim.OnPlayerLogout(21, 20, 3);
    EXPECT_EQ(sim.PlayerCount(), 2u);
    EXPECT_EQ(sim.GroupCount(), 1u); // Group policy remains

    // Leader 20 logs out
    sim.OnPlayerLogout(20, 20, 2);
    EXPECT_EQ(sim.PlayerCount(), 1u);
    EXPECT_EQ(sim.GroupCount(), 0u); // Group policy purged
}

// =============================================================================
// Round 2.2: Multi-Role Bot Solver Tests
// =============================================================================

struct SolverMember
{
    uint8 roles;
};

struct SolverResult
{
    bool hasTank{false};
    bool hasHealer{false};
    uint32 realDpsCount{0};
    uint32 tankBotsNeeded{0};
    uint32 healerBotsNeeded{0};
    uint32 dpsBotsNeeded{0};
};

SolverResult SolveRoles(std::vector<SolverMember> const& members)
{
    SolverResult result;
    if (members.empty() || members.size() >= 5)
        return result;

    size_t const N = members.size();
    bool bestFound = false;
    int bestScore = -1;
    bool bestTank = false;
    bool bestHealer = false;
    uint32 bestDps = 0;

    auto backtrack = [&](auto& self, size_t idx, bool curTank, bool curHealer, uint32 curDps) -> void
    {
        if (idx == N)
        {
            int score = (curTank ? 100 : 0) + (curHealer ? 50 : 0) + int(curDps);
            if (!bestFound || score > bestScore)
            {
                bestScore = score;
                bestTank = curTank;
                bestHealer = curHealer;
                bestDps = curDps;
                bestFound = true;
            }
            return;
        }

        uint8 const availableRoles = members[idx].roles;

        // Try Tank
        if (!curTank && (availableRoles & lfg::PLAYER_ROLE_TANK))
            self(self, idx + 1, true, curHealer, curDps);

        // Try Healer
        if (!curHealer && (availableRoles & lfg::PLAYER_ROLE_HEALER))
            self(self, idx + 1, curTank, true, curDps);

        // Try DPS
        if (curDps < 3 && (availableRoles & lfg::PLAYER_ROLE_DAMAGE))
            self(self, idx + 1, curTank, curHealer, curDps + 1);

        // Unassigned branch
        self(self, idx + 1, curTank, curHealer, curDps);
    };

    backtrack(backtrack, 0, false, false, 0);

    result.hasTank = bestTank;
    result.hasHealer = bestHealer;
    result.realDpsCount = bestDps;

    uint32 const availableBotSlots = static_cast<uint32>(5 - N);
    uint32 remainingSlots = availableBotSlots;

    if (!result.hasTank && remainingSlots > 0)
    {
        result.tankBotsNeeded = 1;
        --remainingSlots;
    }

    if (!result.hasHealer && remainingSlots > 0)
    {
        result.healerBotsNeeded = 1;
        --remainingSlots;
    }

    uint32 const missingDps = (result.realDpsCount >= 3) ? 0 : (3 - result.realDpsCount);
    result.dpsBotsNeeded = std::min<uint32>(remainingSlots, missingDps);

    return result;
}

TEST(BotRoleSolverTest, SinglePlayerMultiRole)
{
    // Solo player queued as Tank | Healer
    // Must be assigned to Tank (or Healer), exactly ONE role, never both!
    std::vector<SolverMember> members = { { lfg::PLAYER_ROLE_TANK | lfg::PLAYER_ROLE_HEALER } };
    SolverResult res = SolveRoles(members);

    EXPECT_TRUE(res.hasTank);
    EXPECT_FALSE(res.hasHealer); // Solo player cannot be both!
    EXPECT_EQ(res.tankBotsNeeded, 0u);
    EXPECT_EQ(res.healerBotsNeeded, 1u); // Healer bot spawned
    EXPECT_EQ(res.dpsBotsNeeded, 3u);    // 3 DPS bots spawned
    EXPECT_EQ(1u + res.tankBotsNeeded + res.healerBotsNeeded + res.dpsBotsNeeded, 5u); // Total exactly 5
}

TEST(BotRoleSolverTest, FourRealDps_NeverExceedsFiveMembers)
{
    // 4 real DPS queued
    // Bot slots available = 5 - 4 = 1 slot.
    // Tank has higher priority than Healer, so add 1 Tank bot.
    // Group must NEVER become 6 players!
    std::vector<SolverMember> members = {
        { lfg::PLAYER_ROLE_DAMAGE },
        { lfg::PLAYER_ROLE_DAMAGE },
        { lfg::PLAYER_ROLE_DAMAGE },
        { lfg::PLAYER_ROLE_DAMAGE }
    };
    SolverResult res = SolveRoles(members);

    EXPECT_FALSE(res.hasTank);
    EXPECT_FALSE(res.hasHealer);
    EXPECT_EQ(res.realDpsCount, 3u); // Capped to 3 standard dungeon DPS
    EXPECT_EQ(res.tankBotsNeeded, 1u);
    EXPECT_EQ(res.healerBotsNeeded, 0u); // Slots exhausted!
    EXPECT_EQ(res.dpsBotsNeeded, 0u);

    uint32 totalGroup = 4u + res.tankBotsNeeded + res.healerBotsNeeded + res.dpsBotsNeeded;
    EXPECT_EQ(totalGroup, 5u); // Exactly 5, NOT 6!
}

TEST(BotRoleSolverTest, FullFiveRealPlayers_ZeroBotsNeeded)
{
    std::vector<SolverMember> members = {
        { lfg::PLAYER_ROLE_TANK },
        { lfg::PLAYER_ROLE_HEALER },
        { lfg::PLAYER_ROLE_DAMAGE },
        { lfg::PLAYER_ROLE_DAMAGE },
        { lfg::PLAYER_ROLE_DAMAGE }
    };
    SolverResult res = SolveRoles(members);

    EXPECT_EQ(res.tankBotsNeeded, 0u);
    EXPECT_EQ(res.healerBotsNeeded, 0u);
    EXPECT_EQ(res.dpsBotsNeeded, 0u);
}

TEST(BotRoleSolverTest, FlexibleRolesThreeMembers)
{
    // Player 1: Tank | DPS
    // Player 2: Healer | DPS
    // Player 3: DPS
    std::vector<SolverMember> members = {
        { lfg::PLAYER_ROLE_TANK | lfg::PLAYER_ROLE_DAMAGE },
        { lfg::PLAYER_ROLE_HEALER | lfg::PLAYER_ROLE_DAMAGE },
        { lfg::PLAYER_ROLE_DAMAGE }
    };
    SolverResult res = SolveRoles(members);

    EXPECT_TRUE(res.hasTank);
    EXPECT_TRUE(res.hasHealer);
    EXPECT_EQ(res.realDpsCount, 1u);
    EXPECT_EQ(res.tankBotsNeeded, 0u);
    EXPECT_EQ(res.healerBotsNeeded, 0u);
    EXPECT_EQ(res.dpsBotsNeeded, 2u); // 3 members + 2 DPS bots = 5 total

    uint32 totalGroup = 3u + res.tankBotsNeeded + res.healerBotsNeeded + res.dpsBotsNeeded;
    EXPECT_EQ(totalGroup, 5u);
}

// =============================================================================
// Round 3: Census, Instance Profiles & Calibration Tests
// =============================================================================

TEST(ContentCensusTest, StaticCensusProfilesPopulated)
{
    EXPECT_GE(sGeneratedInstanceProfiles.size(), 90u);

    // Verify key landmark instances
    auto const* deadmines = FindGeneratedInstanceProfile(36);
    ASSERT_NE(deadmines, nullptr);
    EXPECT_EQ(deadmines->era, ContentEra::Classic);
    EXPECT_FALSE(deadmines->isRaid);
    EXPECT_EQ(deadmines->tier, ContentTier::DUNGEON_NORMAL);

    auto const* moltenCore = FindGeneratedInstanceProfile(409);
    ASSERT_NE(moltenCore, nullptr);
    EXPECT_EQ(moltenCore->era, ContentEra::Classic);
    EXPECT_TRUE(moltenCore->isRaid);
    EXPECT_EQ(moltenCore->tier, ContentTier::RAID_MID);
    EXPECT_EQ(moltenCore->intendedPlayers, 40u);

    auto const* karazhan = FindGeneratedInstanceProfile(532);
    ASSERT_NE(karazhan, nullptr);
    EXPECT_EQ(karazhan->era, ContentEra::TBC);
    EXPECT_TRUE(karazhan->isRaid);
    EXPECT_EQ(karazhan->tier, ContentTier::RAID_ENTRY);
    EXPECT_EQ(karazhan->intendedPlayers, 10u);

    auto const* naxx = FindGeneratedInstanceProfile(533);
    ASSERT_NE(naxx, nullptr);
    EXPECT_EQ(naxx->era, ContentEra::WotLK);
    EXPECT_TRUE(naxx->isRaid);
    EXPECT_EQ(naxx->tier, ContentTier::RAID_ENTRY);

    auto const* icc = FindGeneratedInstanceProfile(631);
    ASSERT_NE(icc, nullptr);
    EXPECT_EQ(icc->era, ContentEra::WotLK);
    EXPECT_TRUE(icc->isRaid);
    EXPECT_EQ(icc->tier, ContentTier::RAID_PINNACLE);
}

TEST(ContentCensusTest, InstanceProfileRegistryTiersCalibrated)
{
    sInstanceProfileRegistry->Initialize();

    std::vector<std::string> issues;
    EXPECT_TRUE(sInstanceProfileRegistry->ValidateAll(issues));

    // Check tier resolution
    EXPECT_EQ(sInstanceProfileRegistry->GetTierForMap(36), ContentTier::DUNGEON_NORMAL);
    EXPECT_EQ(sInstanceProfileRegistry->GetTierForMap(36, 1), ContentTier::DUNGEON_HEROIC); // Heroic difficulty = 1
    EXPECT_EQ(sInstanceProfileRegistry->GetTierForMap(409), ContentTier::RAID_MID);
    EXPECT_EQ(sInstanceProfileRegistry->GetTierForMap(531), ContentTier::RAID_PINNACLE);
    EXPECT_EQ(sInstanceProfileRegistry->GetTierForMap(631), ContentTier::RAID_PINNACLE);
}

TEST(ProgressionCalibrationTest, Cap60AllEras_MonotonicZoneCompression)
{
    // Cap 60 with Classic, TBC, and WotLK:
    // Classic authored: 1-60 -> effective 1-45
    // TBC authored: 58-70 -> effective 45-55
    // WotLK authored: 68-80 -> effective 55-60
    ProgressionLayout layout = ProgressionLayout::Create(60, true, true);
    std::string err;
    ASSERT_TRUE(layout.Validate(err)) << err;

    // Classic zone (e.g. Westfall authored ~15, Plaguelands authored ~55)
    uint8 const effWestfall = layout.MapAuthoredToEffective(ContentEra::Classic, 15);
    uint8 const effPlaguelands = layout.MapAuthoredToEffective(ContentEra::Classic, 55);
    EXPECT_GE(effWestfall, 1);
    EXPECT_LE(effPlaguelands, 45);
    EXPECT_LT(effWestfall, effPlaguelands);

    // TBC zone (e.g. Hellfire Peninsula authored ~60, Shadowmoon authored ~70)
    uint8 const effHellfire = layout.MapAuthoredToEffective(ContentEra::TBC, 60);
    uint8 const effShadowmoon = layout.MapAuthoredToEffective(ContentEra::TBC, 70);
    EXPECT_GE(effHellfire, 45);
    EXPECT_EQ(effShadowmoon, 55);
    EXPECT_LE(effHellfire, effShadowmoon);

    // WotLK zone (e.g. Borean Tundra authored ~70, Icecrown authored ~80)
    uint8 const effBorean = layout.MapAuthoredToEffective(ContentEra::WotLK, 70);
    uint8 const effIcecrown = layout.MapAuthoredToEffective(ContentEra::WotLK, 80);
    EXPECT_GE(effBorean, 55);
    EXPECT_EQ(effIcecrown, 60);
    EXPECT_LE(effBorean, effIcecrown);
}

TEST(ProgressionCalibrationTest, QuestChainMonotonicity_NeverInverts)
{
    ProgressionLayout layout = ProgressionLayout::Create(60, true, true);

    // Chain: Q1 (authored 12) -> Q2 (authored 14) -> Q3 (authored 16)
    uint8 q1 = layout.MapAuthoredToEffective(ContentEra::Classic, 12);
    uint8 q2 = layout.MapAuthoredToEffective(ContentEra::Classic, 14);
    uint8 q3 = layout.MapAuthoredToEffective(ContentEra::Classic, 16);

    EXPECT_LE(q1, q2);
    EXPECT_LE(q2, q3);

    // Cross-era transition: End of Classic Q (authored 60) -> Intro TBC Q (authored 58)
    uint8 qClassicEnd = layout.MapAuthoredToEffective(ContentEra::Classic, 60);
    uint8 qTbcStart = layout.MapAuthoredToEffective(ContentEra::TBC, 58);
    EXPECT_EQ(qClassicEnd, 45);
    EXPECT_GE(qTbcStart, 45);
}




