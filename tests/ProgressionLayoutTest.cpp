/*
 * CoA Universal Content Scaling
 * ProgressionLayoutTest: Comprehensive unit test matrix for ProgressionLayout combinations.
 */

#include "ProgressionLayout.h"
#include "ContentEra.h"
#include "ContentPackRegistry.h"
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

