/*
 * CoA Universal Content Scaling
 * CoAContentScaling: Master controller and lifecycle manager.
 */

#ifndef COA_CONTENT_SCALING_H
#define COA_CONTENT_SCALING_H

#include "ContentEra.h"
#include "ContentTier.h"
#include "Define.h"
#include "DungeonFinding/LFG.h"
#include "InstanceScaleContext.h"
#include "ObjectGuid.h"
#include "ProgressionLayout.h"
#include <mutex>
#include <string>
#include <unordered_map>

class Creature;
class Player;
class Unit;
class Quest;
class Map;
class InstanceMap;
class Group;
struct CreatureTemplate;

namespace lfg
{
    struct LfgProposal;
}

struct PlayerLfgSettings
{
    lfg::LfgCompositionMode compositionMode{lfg::LfgCompositionMode::MATCHMAKING};
    uint32 challengeSize{0}; // 0 = Adaptive, 1 = Solo, 2..40 = fixed
};

struct PendingInstanceScalePolicy
{
    uint32 mapId{0};
    ObjectGuid groupGuid;
    uint32 challengeSize{0};
    lfg::LfgCompositionMode compositionMode{lfg::LfgCompositionMode::MATCHMAKING};
    uint64 generation{0};
    uint32 createdAt{0}; // GameTime::GetGameTime()
};

class CoAContentScaling
{
public:
    static CoAContentScaling* Instance();

    void LoadConfig();
    void LoadReloadableConfig();
    void FinalizeAndInitialize();
    void InitializeLayout();

    [[nodiscard]] bool IsEnabled() const { return _enabled; }
    [[nodiscard]] bool IsWorldLeechEnabled() const { return _worldLeechEnabled; }
    [[nodiscard]] float GetWorldLeechPercent() const { return _worldLeechPercent; }
    [[nodiscard]] bool IsGroupScalingEnabled() const { return _groupScalingEnabled; }
    [[nodiscard]] bool IsLockOnEncounterStartEnabled() const { return _lockOnEncounterStart; }
    [[nodiscard]] float GetDamageMultiplier() const { return _damageMultiplier; }

    // Configured health multiplier for a creature of this kind: the place (open world, dungeon, raid) times the role (boss or ordinary mob).
    [[nodiscard]] float GetHealthMultiplier(bool dungeon, bool raid, bool boss) const
    {
        float const place = raid ? _healthRaidMultiplier : (dungeon ? _healthDungeonMultiplier : _healthWorldMultiplier);
        return place * (boss ? _healthBossMultiplier : _healthTrashMultiplier);
    }
    [[nodiscard]] bool IsAdaptiveMechanicsEnabled() const { return _adaptiveMechanicsEnabled; }
    [[nodiscard]] bool IsRemoveClassRestrictionsEnabled() const { return _removeClassRestrictions; }
    [[nodiscard]] bool IsDebugEnabled() const { return _debug; }

    void SetEnabled(bool enabled) { _enabled = enabled; }
    void SetDebugEnabled(bool enabled) { _debug = enabled; }
    void SetGroupScalingEnabled(bool enabled) { _groupScalingEnabled = enabled; }
    void SetLockOnEncounterStart(bool enabled) { _lockOnEncounterStart = enabled; }
    void SetAdaptiveMechanicsEnabled(bool enabled) { _adaptiveMechanicsEnabled = enabled; }

    [[nodiscard]] ProgressionLayout const& GetLayout() const { return _layout; }

    // Effective Level Mapping
    [[nodiscard]] uint8 GetEffectiveCreatureLevel(CreatureTemplate const* cinfo, Creature const* creature, uint8 authoredLevel) const;
    [[nodiscard]] int32 GetEffectiveQuestLevel(Quest const* quest) const;
    [[nodiscard]] uint32 GetEffectiveQuestMinLevel(Quest const* quest) const;

    // Production Hook Lifecycle
    void RegisterLocalLevelScalingHooks();
    void UnregisterLocalLevelScalingHooks();

    // Combat scaling ownership. While the module is enabled it builds the whole combat budget (health, damage, armor)
    // of dungeon and raid creatures, so it is the single authority there. The core's CoA.CreatureScaling asks this
    // (see CreatureScaling::SetCombatAuthority) and applies its Dungeon, Raid and Map* multipliers only to creatures
    // the module does not own; open world creatures stay with CoA.CreatureScaling.World.* and Local Level Scaling.
    [[nodiscard]] bool OwnsCombatScaling(Creature const* creature) const;
    [[nodiscard]] static bool IsCombatScalable(Creature const* creature);

    // Every creature the module gives a combat budget (any map) gets its whole health from that budget, which already
    // contains CoA.CreatureScaling's multiplier exactly once (World.Health in the open world). The core's own health
    // hook asks this (CreatureScaling::SetHealthBudgetAuthority) and skips such creatures, so the world multiplier is
    // applied once whichever of the two level selection hooks runs first.
    [[nodiscard]] bool BuildsHealthBudget(Creature const* creature) const;

    // Combat scaling calculations
    void ApplyCreatureScaling(CreatureTemplate const* cinfo, Creature* creature);
    void RecalculateEncounterCombatStats(Creature* boss, EncounterScaleSnapshot const& snapshot,
                                         EncounterHealthTransferPolicy hpPolicy = EncounterHealthTransferPolicy::FULL_ON_PULL);
    void RefreshDungeonCreatures(Map* map);

    // Expansion enablement
    void SetTbcEnabled(bool enabled) { _tbcEnabled = enabled; }
    void SetWotlkEnabled(bool enabled) { _wotlkEnabled = enabled; }
    [[nodiscard]] bool IsTbcEnabled() const { return _tbcEnabled; }
    [[nodiscard]] bool IsWotlkEnabled() const { return _wotlkEnabled; }

    // Map access validation
    [[nodiscard]] bool CanPlayerEnterMap(Player const* player, uint32 mapId, uint8 difficulty = 0) const;

    // LFG Reward Level Resolution
    [[nodiscard]] uint8 ResolveLfgRewardLevel(Player const* player, uint32 dungeonId, uint8 playerLevel) const;

    // LFG Policy & Composition Integration
    void SetPlayerLfgMode(ObjectGuid guid, lfg::LfgCompositionMode mode);
    [[nodiscard]] lfg::LfgCompositionMode GetPlayerLfgMode(ObjectGuid guid) const;
    bool SetPlayerLfgChallenge(ObjectGuid guid, uint32 challengeSize);
    [[nodiscard]] uint32 GetPlayerLfgChallenge(ObjectGuid guid) const;
    void LoadPlayerLfgSettings(Player* player);
    void SavePlayerLfgSettings(Player* player);
    void OnPlayerLogout(Player* player);

    void OnResolveLfgQueuePolicy(ObjectGuid const& guid, lfg::LfgQueuePolicy& policy);
    void OnLfgProposalMadeGroup(lfg::LfgProposal const& proposal, Group* group);
    void OnInstanceMapCreated(InstanceMap* instanceMap, Player* player);

    // Pending Policy Lifecycle & Diagnostics
    std::optional<PendingInstanceScalePolicy> ConsumePendingInstancePolicy(uint32 mapId, ObjectGuid groupGuid, ObjectGuid playerGuid);
    void PurgeExpiredPendingPolicies();
    [[nodiscard]] size_t GetPendingGroupPoliciesCount() const;
    [[nodiscard]] size_t GetPendingPlayerPoliciesCount() const;
    std::vector<PendingInstanceScalePolicy> GetAllPendingPolicies() const;

private:
    CoAContentScaling() = default;
    void ApplyCombatBudget(CreatureTemplate const* cinfo, Creature* creature, float groupHealthScale,
                           float groupDamageScale, float effectivePlayers, EncounterHealthTransferPolicy hpPolicy);

    bool _enabled{true};
    bool _tbcEnabled{false};
    bool _wotlkEnabled{false};

    bool _groupScalingEnabled{true};
    bool _lfgAllowPartialGroups{true};
    bool _worldLeechEnabled{false};
    float _worldLeechPercent{5.0f};
    float _damageMultiplier{1.0f};
    float _healthWorldMultiplier{1.0f};
    float _healthDungeonMultiplier{1.0f};
    float _healthRaidMultiplier{1.0f};
    float _healthTrashMultiplier{1.0f};
    float _healthBossMultiplier{1.0f};
    void LoadHealthMultipliers();
    bool _lockOnEncounterStart{true};
    bool _adaptiveMechanicsEnabled{true};
    bool _removeClassRestrictions{false};
    bool _scaleLootCount{true};
    bool _allowSoloRaids{true};
    bool _debug{false};

    std::string _progressionMode{"Auto"};
    uint8 _customClassicEnd{0};
    uint8 _customTbcEnd{0};

    ProgressionLayout _layout;

    lfg::LfgCompositionMode _defaultLfgCompositionMode{lfg::LfgCompositionMode::MATCHMAKING};
    uint32 _defaultLfgChallengeSize{0};

    mutable std::mutex _lfgSettingsLock;
    std::unordered_map<ObjectGuid, PlayerLfgSettings> _playerLfgSettings;

    mutable std::mutex _pendingPolicyLock;
    uint64 _pendingPolicyGeneration{0};
    std::unordered_map<ObjectGuid, PendingInstanceScalePolicy> _pendingInstancePolicies;
    std::unordered_map<ObjectGuid, PendingInstanceScalePolicy> _pendingPlayerPolicies;
};

#define sCoAContentScaling CoAContentScaling::Instance()

#endif // COA_CONTENT_SCALING_H
