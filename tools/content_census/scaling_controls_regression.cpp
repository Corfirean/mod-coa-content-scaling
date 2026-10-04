#include "ContentPackRegistry.h"
#include "GeneratedContentCensus.h"
#include "InstanceProfile.h"
#include "ItemBudgetScaler.h"
#include "Log.h"
#include "ProgressionRewardResolver.h"
#include <atomic>
#include <iostream>
#include <mutex>
#include <unordered_map>

InstanceProfileRegistry::InstanceProfileRegistry() = default;
InstanceProfileRegistry* InstanceProfileRegistry::Instance()
{
    static InstanceProfileRegistry registry;
    return &registry;
}
std::optional<ContentEra> InstanceProfileRegistry::GetEraForMap(uint32, uint8) const { return std::nullopt; }
// ACTUAL_KEYS
constexpr unsigned MAX_ITEM_PROTO_SPELLS = 5;
constexpr unsigned MAX_ITEM_PROTO_SOCKETS = 3;
struct ItemTemplate
{
    uint32 ItemId = 768, ItemLevel = 9, RequiredLevel = 4, ItemSet = 0;
    struct { uint32 SpellId = 0, SpellTrigger = 0; } Spells[MAX_ITEM_PROTO_SPELLS];
    struct { uint32 Color = 0; } Socket[MAX_ITEM_PROTO_SOCKETS];
};
struct Map { uint32 GetId() const { return 0; } };
struct Creature
{
    Map* GetMap() const { return nullptr; }
    uint32 GetAreaId() const { return 12; }
};
struct CreatureTemplate { uint32 Entry = 30; uint8 expansion = 0; };
struct Quest
{
    int32 GetQuestLevel() const { return 43; }
    uint32 GetMinLevel() const { return 39; }
    uint32 GetQuestId() const { return 8325; }
    int32 GetZoneOrSort() const { return 3431; }
};
struct Config
{
    bool scaleItems = true;
    template <typename T> T GetOption(char const*, T) { return T(scaleItems); }
} config;
Config* sConfigMgr = &config;
constexpr unsigned CONFIG_MAX_PLAYER_LEVEL = 0;
struct World
{
    uint32 cap = 60;
    unsigned reads = 0;
    uint32 getIntConfig(unsigned) { ++reads; return cap; }
} world;
World* sWorld = &world;
struct InstanceScalingMgr
{
    unsigned loads = 0;
    std::unordered_map<uint64, unsigned> _contexts, _challengeSizes, _compositionModes, _bossFlexCache;
    std::mutex _lock;
    void LoadCalibratedBossFlex() { ++loads; }
    void RemoveMapContext(uint32 mapId, uint32 instanceId);
} instances;
InstanceScalingMgr* sInstanceScalingMgr = &instances;
unsigned itemMutations = 0;
ItemBudgetScaler* ItemBudgetScaler::Instance() { static ItemBudgetScaler scaler; return &scaler; }
void ItemBudgetScaler::ScaleAllItems(ProgressionLayout const&) { ++itemMutations; }
namespace LocalLevelScaling
{
    std::atomic<bool> ContentScalingActive{true};
    std::atomic<void*> QuestBaseLevelOwner{reinterpret_cast<void*>(1)}, QuestMinLevelOwner{reinterpret_cast<void*>(1)},
        CreatureBaseLevelOwner{reinterpret_cast<void*>(1)}, QuestMoneyMaxLevelOwner{reinterpret_cast<void*>(1)},
        KillContentLevelOwner{reinterpret_cast<void*>(1)}, QuestRewardRateOwner{reinterpret_cast<void*>(1)};
}
using ObjectGuid = unsigned;
struct Group { uint8 count = 2; uint8 GetMembersCount() const { return count; } };
struct Player { Group* group = nullptr; Group* GetGroup() { return group; } } queuePlayer;
namespace ObjectAccessor { Player* FindPlayer(ObjectGuid) { return &queuePlayer; } }
namespace lfg
{
    enum class LfgCompositionMode { MATCHMAKING, BOT_FILL, CURRENT_PARTY };
    struct LfgQueuePolicy
    {
        LfgCompositionMode compositionMode = LfgCompositionMode::MATCHMAKING;
        uint32 challengeSize = 0;
        bool bypassMatchmaking = false, requireStandardRoles = true;
        uint8 minPlayers = 5, targetPlayers = 5;
    };
}
struct ScriptMgr { bool provider = false; bool HasLfgAutoFillProvider() { return provider; } } scripts;
ScriptMgr* sScriptMgr = &scripts;
struct PlayerLfgSettings
{
    lfg::LfgCompositionMode compositionMode = lfg::LfgCompositionMode::CURRENT_PARTY;
    uint32 challengeSize = 20;
};
class CoAContentScaling
{
public:
    bool _enabled = false;
    bool _groupScalingEnabled = true, _lfgAllowPartialGroups = true;
    unsigned hooks = 0;
    bool _tbcEnabled = true, _wotlkEnabled = true;
    std::string _progressionMode = "Auto";
    uint8 _customClassicEnd = 0, _customTbcEnd = 0;
    ProgressionLayout _layout = ProgressionLayout::Create(60, true, true);
    std::mutex _lfgSettingsLock;
    std::unordered_map<ObjectGuid, PlayerLfgSettings> _playerLfgSettings;
    lfg::LfgCompositionMode _defaultLfgCompositionMode = lfg::LfgCompositionMode::CURRENT_PARTY;
    uint32 _defaultLfgChallengeSize = 20;
    void FinalizeAndInitialize();
    void InitializeLayout();
    void RegisterLocalLevelScalingHooks() { ++hooks; }
    void UnregisterLocalLevelScalingHooks();
    void OnResolveLfgQueuePolicy(ObjectGuid const&, lfg::LfgQueuePolicy&);
    uint8 GetEffectiveCreatureLevel(CreatureTemplate const*, Creature const*, uint8) const;
    int32 GetEffectiveQuestLevel(Quest const*) const;
    uint32 GetEffectiveQuestMinLevel(Quest const*) const;
};
// ACTUAL_METHODS

int main()
{
    unsigned failures = 0;
    auto check = [&failures](bool ok, char const* name)
    {
        if (!ok)
        {
            ++failures;
            std::cerr << "FAIL: " << name << '\n';
        }
    };
    auto* registry = sContentPackRegistry;
    registry->Clear();
    CoAContentScaling disabled;
    disabled.FinalizeAndInitialize();
    check(registry->IsFinalized(), "disabled lifecycle remains finalized for restart-only enable changes");
    check(world.reads == 0 && instances.loads == 0 && itemMutations == 0 && disabled.hooks == 0,
          "disabled startup must not initialize scaling or mutate items");
    check(!LocalLevelScaling::ContentScalingActive && !LocalLevelScaling::QuestBaseLevelOwner &&
          !LocalLevelScaling::QuestMinLevelOwner && !LocalLevelScaling::CreatureBaseLevelOwner &&
          !LocalLevelScaling::QuestMoneyMaxLevelOwner && !LocalLevelScaling::KillContentLevelOwner &&
          !LocalLevelScaling::QuestRewardRateOwner, "disabled startup clears every local scaling callback");
    lfg::LfgQueuePolicy policy;
    disabled.OnResolveLfgQueuePolicy(1, policy);
    check(policy.compositionMode == lfg::LfgCompositionMode::MATCHMAKING && policy.challengeSize == 0 &&
          !policy.bypassMatchmaking && policy.requireStandardRoles && policy.minPlayers == 5 && policy.targetPlayers == 5,
          "disabled module must preserve the original LFG policy");
    CreatureTemplate creature;
    Quest quest;
    check(disabled.GetEffectiveCreatureLevel(&creature, nullptr, 45) == 45, "disabled creature level is unchanged");
    check(disabled.GetEffectiveQuestLevel(&quest) == 43 && disabled.GetEffectiveQuestMinLevel(&quest) == 39,
          "disabled quest and minimum levels are unchanged");
    registry->Clear();
    CoAContentScaling enabled;
    enabled._enabled = true;
    enabled.FinalizeAndInitialize();
    enabled.FinalizeAndInitialize();
    check(world.reads == 1 && instances.loads == 1 && itemMutations == 0 && enabled.hooks == 1,
          "enabled startup initializes once without eagerly mutating item templates");
    enabled.OnResolveLfgQueuePolicy(1, policy);
    check(policy.compositionMode == lfg::LfgCompositionMode::CURRENT_PARTY && policy.challengeSize == 20 &&
          policy.minPlayers == 1 && !policy.requireStandardRoles, "enabled LFG control remains active");
    enabled._defaultLfgCompositionMode = lfg::LfgCompositionMode::MATCHMAKING;
    Group party;
    for (bool provider : {false, true})
    {
        scripts.provider = provider;
        for (uint8 count = 1; count <= 5; ++count)
        {
            party.count = count;
            queuePlayer.group = count == 1 ? nullptr : &party;
            enabled.OnResolveLfgQueuePolicy(1, policy);
            check(policy.bypassMatchmaking && !policy.requireStandardRoles && policy.minPlayers == count &&
                policy.targetPlayers == count, "partial parties enter with or without a bot provider");
        }
    }
    queuePlayer.group = nullptr;
    for (unsigned flag = 0; flag < 2; ++flag)
    {
        enabled._groupScalingEnabled = flag != 0;
        enabled._lfgAllowPartialGroups = flag == 0;
        enabled.OnResolveLfgQueuePolicy(1, policy);
        check(!policy.bypassMatchmaking && policy.requireStandardRoles && policy.targetPlayers == 5,
            "disabled scaling or partial-group option preserves matchmaking");
    }
    enabled._groupScalingEnabled = enabled._lfgAllowPartialGroups = true;
    enabled._defaultLfgCompositionMode = lfg::LfgCompositionMode::BOT_FILL;
    enabled.OnResolveLfgQueuePolicy(1, policy);
    check(policy.compositionMode == lfg::LfgCompositionMode::BOT_FILL && !policy.bypassMatchmaking,
        "registered bot provider keeps BotFill");
    scripts.provider = false;
    enabled.OnResolveLfgQueuePolicy(1, policy);
    check(policy.bypassMatchmaking, "BotFill without provider falls back to current party with scaling");
    enabled._groupScalingEnabled = false;
    enabled.OnResolveLfgQueuePolicy(1, policy);
    check(!policy.bypassMatchmaking, "BotFill without provider uses matchmaking without scaling");
    registry->Clear();
    world.cap = 50;
    CoAContentScaling invalid;
    invalid._enabled = true;
    LocalLevelScaling::ContentScalingActive = true;
    LocalLevelScaling::QuestBaseLevelOwner = reinterpret_cast<void*>(1);
    invalid.FinalizeAndInitialize();
    invalid.FinalizeAndInitialize();
    check(!invalid._enabled && invalid.hooks == 0 && instances.loads == 1 && itemMutations == 0,
          "invalid layout stops startup before boss loading, item mutation and hook registration");
    check(world.reads == 2 && registry->IsFinalized() && !LocalLevelScaling::ContentScalingActive &&
          !LocalLevelScaling::QuestBaseLevelOwner, "invalid startup clears callbacks and cannot retry initialization");
    world.cap = 60;
    uint64 const removedKey = (uint64(530) << 32) | 42;
    uint64 const otherInstance = (uint64(530) << 32) | 43;
    uint64 const otherMap = (uint64(571) << 32) | 42;
    for (auto key : {removedKey, otherInstance, otherMap})
    {
        instances._contexts[key] = 1;
        instances._challengeSizes[key] = 5;
        instances._compositionModes[key] = 1;
    }
    instances._bossFlexCache[123] = 1;
    instances.RemoveMapContext(530, 42);
    instances.RemoveMapContext(530, 42);
    check(!instances._contexts.contains(removedKey) && !instances._challengeSizes.contains(removedKey) &&
          !instances._compositionModes.contains(removedKey), "destroyed map drops encounter and group state");
    check(instances._contexts.contains(otherInstance) && instances._contexts.contains(otherMap) &&
          instances._challengeSizes.size() == 2 && instances._compositionModes.size() == 2 &&
          instances._bossFlexCache.contains(123), "map cleanup preserves other instances and boss calibration");
    auto stock = ProgressionLayout::Create(80, true, true);
    auto custom = ProgressionLayout::Create(80, true, true, 45, 55);
    std::string error;
    check(stock.IsStockIdentity() && !custom.IsStockIdentity() && custom.Validate(error),
          "custom cap80 is valid but does not preserve stock budgets");
    auto* rewards = ProgressionRewardResolver::Instance();
    check(rewards->ResolveItemRequiredLevel(60, ContentEra::Classic, 45, custom) == 45 &&
          rewards->ResolveEffectiveAccessMin(ContentEra::Classic, ContentTier::DUNGEON_NORMAL, 60, custom) == 45,
          "custom cap80 maps item requirements and dungeon access with creature levels");
    check(rewards->ResolveQuestXP(10000, 60, 45, custom, ContentEra::Classic) == 8625 &&
          rewards->ResolveLfgRewardLevel(ContentEra::Classic, 45, custom) == 60,
          "custom cap80 calibrates quest XP and LFG reward lookup");
    check(rewards->ResolveItemRequiredLevel(60, ContentEra::Classic, 60, stock) == 60 &&
          rewards->ResolveEffectiveAccessMin(ContentEra::Classic, ContentTier::DUNGEON_NORMAL, 60, stock) == 60 &&
          rewards->ResolveQuestXP(10000, 60, 60, stock, ContentEra::Classic) == 10000 &&
          rewards->ResolveLfgRewardLevel(ContentEra::Classic, 45, stock) == 45,
          "stock cap80 preserves authored requirements and rewards");
    registry->Clear();
    ItemTemplate item;
    auto original = ItemScalingContext::Resolve(&item);
    registry->RegisterItemOverride(item.ItemId, ContentEra::WotLK);
    auto overridden = ItemScalingContext::Resolve(&item);
    check(overridden.era == ContentEra::WotLK && overridden.hasGeneratedProfile, "item override beats census at runtime");
    check(original.tier == overridden.tier && original.sourceMap == overridden.sourceMap &&
          original.specialFlags == overridden.specialFlags && original.policy == overridden.policy,
          "era override retains the item's tier and safety policy");
    registry->Clear();
    unsigned preservedProfiles = 0;
    for (auto const& profile : sGeneratedItemProfiles)
    {
        if (profile.policy != uint8(ItemScalingPolicy::PRESERVE))
            continue;
        ++preservedProfiles;
        item.ItemId = profile.itemId;
        registry->RegisterItemOverride(item.ItemId, ContentEra::TBC);
        auto ctx = ItemScalingContext::Resolve(&item);
        check(ctx.era == ContentEra::TBC && ctx.policy == ItemScalingPolicy::PRESERVE,
              "explicit era override cannot remove PRESERVE protection");
        registry->Clear();
    }
    check(preservedProfiles > 0, "PRESERVE fixtures exist");
    std::cout << "Scaling controls regression: " << failures << " failures\n";
    return failures ? 1 : 0;
}
