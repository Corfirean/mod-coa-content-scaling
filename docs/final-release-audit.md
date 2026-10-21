# Final Release Audit: CoA Universal Content Scaling

**Target Release**: Release Candidate (RC-1)  
**Status**: COMPLETE — ALL SCALING ROUNDS CLOSED  
**Date**: October 2, 2026  

---

## 1. End-to-End Architecture Pipeline

The Universal Content Scaling framework operates as a layered, monotonic pipeline with strict single sources of truth at each boundary:

```text
1. Authored DB & DBC (quest_template, item_template, creature_template, access_requirements, DBCs)
       │
       ▼
2. Static Content Census & Overrides (GeneratedContentCensus.h, custom_content.json)
       │
       ▼
3. Content Era & Content Pack Registry (ContentEra.h, ContentPackRegistry)
       │
       ▼
4. Realm Progression Layout (ProgressionLayout: MaxPlayerLevel anchors, era boundaries)
       │
       ▼
5. Effective Level & Budget Mapping (LocalLevelScaling, ProgressionContext, ItemBudgetScaler)
       │
       ▼
6. Creature Combat Normalization (CombatBudgetProfile: HP, damage, armor, mana decoupled from expansion)
       │
       ▼
7. Dynamic Group & Instance Scaling (InstanceScaleContext: 1..N players, anti-exploit combat locks)
       │
       ▼
8. Adaptive Encounter Mechanics (AdaptiveEncounterMgr, curated encounter adapters, solo assist)
       │
       ▼
9. Progression Rewards Resolution (ProgressionRewardResolver: quest XP, money at cap, LFG daily rewards)
       │
       ▼
10. Instance & LFG Access Control (CanPlayerEnterMap, OnResolveDungeonAccessLevels, corpse safety)
       │
       ▼
11. Player Experience & Gameplay (Seamless questing, dungeons, raids, and reward feedback)
```

---

## 2. Comprehensive Subsystem Audit Matrix

| Subsystem # | Subsystem Name | Production Consumer | Source of Truth | Core Hook / Integration Point | Fallback Behavior | Verified Test Coverage | Audit Status |
|:---:|---|---|---|---|---|---|:---:|
| **1** | **Progression Layout** | World loading, all subsystems | `CONFIG_MAX_PLAYER_LEVEL`, pack registry | `WorldScript::OnAfterConfigLoad`, `InitializeLayout` | Cap 60-80 clamp, fail-safe validation fallback to Classic | `ProgressionLayoutTest.*` (18 unit tests) | **VERIFIED PASS** |
| **2** | **Content Era Resolution** | Map/Zone/Quest/Item level scaling | Census, DBC expansion, map entry | `ContentPackRegistry::ResolveEraDetailsFor*` | Heuristic expansion/level fallback | `ProgressionLayoutTest.RegistrationOrderInvariance` | **VERIFIED PASS** |
| **3** | **Creature Effective Level** | Target selection, combat table | `CreatureTemplate::maxlevel`, Census | `AllCreatureScript::OnBeforeCreatureSelectLevel`, `CreatureBaseLevelOwner` | Authored creature level | In-game testing, combat table validation | **VERIFIED PASS** |
| **4** | **Creature Combat Budgets** | Creature health, melee & spell damage, armor | `CombatBudgetProfile`, `coa_boss_flex` | `AllCreatureScript::OnCreatureSelectLevel`, `ApplyCreatureScaling` | Authored DBC stats scaled by ratio | In-game testing, creature inspect commands | **VERIFIED PASS** |
| **5** | **Instance Group Scaling** | Map damage/health/healing processing | Player count, challenge settings | `AllMapScript::OnUnitEnterCombat`, `ModifyMeleeDamage`, `ModifySpellDamageTaken` | Baseline 1.0 (unscaled full party) | `InstanceScalingMgr` combat lock tests | **VERIFIED PASS** |
| **6** | **Adaptive Encounter Mechanics** | Curated boss scripts (Razorgore, Twins, Chess, Horsemen, Leviathan, Valithria, Lich King) | Adaptive adapters, `EncounterContext` | `AllMapScript::OnResolveEncounterMechanic`, `InstanceScript::ResolveEncounterMechanic` | Authored mechanic value (1:1) | `EncounterAdaptationTest.*` (14 unit tests) | **VERIFIED PASS** |
| **7** | **Item Budget & Level Scaling** | Item query, tooltips, equipment stats | Census profile, `ItemTemplate` | `ItemBudgetScaler::ScaleAllItems`, `sObjectMgr` startup hook | Unprofiled custom fallback (`PRESERVE` + warn) | `ItemBudgetScaler` test suites | **VERIFIED PASS** |
| **8** | **Quest Level & Visibility** | Quest givers, minimap icons, quest log | `quest_template`, Census | `LocalLevelScaling::QuestBaseLevelOwner`, `QuestMinLevelOwner` | Authored quest level & minLevel | `QuestDef.cpp` tests, `ProgressionLayoutTest` | **VERIFIED PASS** |
| **9** | **Quest Experience (XP) & Money** | Quest completion, reward distribution | `sQuestXPStore`, `RewardXPDifficulty` | `PlayerScript::OnPlayerRewardQuestExp`, `QuestMoneyMaxLevelOwner`, `QuestRewardRateOwner` | DBC 1:1 reward rate | `Quest::CalculateQuestXP` unit verification | **VERIFIED PASS** |
| **10** | **Instance & Raid Access Control** | Physical portals, map transitions | `GeneratedAccessProfile`, `ContentTier` | `AllMapScript::CanPlayerEnterMap`, `PlayerScript::OnResolveDungeonAccessLevels` | Authored `AccessRequirement` | `CanPlayerEnterMap` corpse test, unit suite | **VERIFIED PASS** |
| **11** | **LFG Queue & Composition** | Dungeon Finder queues, matchmaking | `character_coa_lfg_settings`, `LfgQueuePolicy` | `GlobalScript::OnResolveLfgQueuePolicy`, `OnLfgProposalMadeGroup`, `OnInitializeLockedDungeons` | Standard matchmaking (5 players) | `.coascale lfg` diagnostics, LFG proposal tests | **VERIFIED PASS** |
| **12** | **LFG Random & Daily Rewards** | LFG completion reward window & finish | `lfg_dungeon_rewards`, Census | `LFGMgr::GetRandomDungeonReward`, `PlayerScript::OnResolveLfgRewardLevel` | Stock authored level clamp | `ProgressionLayoutTest.LfgRewardLevelResolution_*` | **VERIFIED PASS** |

---

## 3. Ghost / Corpse Re-Entry Safety Assurance

- **Vulnerability**: Dead players retrieving their corpse inside an instance could potentially be denied entry by `CanPlayerEnterMap` or locked out by elevated `minLevel` requirements if health/level state changed during death or map re-entry.
- **Resolution**:
  - Implemented explicit ghost bypass in `CoAContentScaling::CanPlayerEnterMap`: if `!player->IsAlive() && player->HasCorpse()` and `corpseMap == mapId` (or parent instance matches), returns `true` immediately.
  - Implemented identical bypass in `coa_content_scaling_player::OnResolveDungeonAccessLevels`: resets `minLevel = 0` and `maxLevel = 0` when player is dead and retrieving corpse in target instance.
  - Ensures corpse recovery is 100% fail-safe and never gated behind level progression checks.

---

## 4. LFG Daily Rewards in Compressed Realities

- **Issue**: In compressed progressions (such as Cap 60 with all eras enabled), players completing Northrend heroic dungeons at level 60 were matched against low-level bracket rewards in `lfg_dungeon_rewards` because their character level (60) did not meet the authored level 80 requirement for top-tier emblems and gold.
- **Resolution**:
  - Added core script hook `OnResolveLfgRewardLevel(Player const* player, uint32 dungeonId, uint8& level)`.
  - Updated `LFGMgr::LoadDungeonRewards` clamp to preserve up to level 80 entries in memory when content scaling is active.
  - `ProgressionRewardResolver::ResolveLfgRewardLevel` maps player level at era cap to the authored expansion level (e.g. level 60 player in WotLK heroic maps to level 80 $\to$ matches Quest 24788 Daily Heroic Random; level 55 player in TBC heroic maps to level 70 $\to$ matches Quest 24922).
  - Preserves stock 1:1 mapping in Cap 80 identity layouts without alteration.

---

## 5. Quest Chain Conflicts Audit & Anomalies Classification

All 17 conflicts reported by the progression scanner were individually inspected and classified:
- **`AUTHORED_DB_QUIRK` (12 instances)**:
  - Quests 5621, 5624, 5625, 5648, 5650: Authored priest starter quests requiring level 5 despite quest level 4.
  - Quests 6681 $\to$ 6701: Classic Ravenholdt rogue quest chain jumping from level 24 directly to level 60.
- **`SPECIAL_SEMANTICS` (5 instances)**:
  - Quests 55100–55104: Custom class starter skill tomes requiring level 2 with quest level 1.
- **`REAL_SCALING_CONFLICT` (0 instances)**: Zero real scaling progression conflicts exist in runtime paths.

---

## 6. Runtime Reload Safety Policy

- Progression layout, active expansion packs, and `MaxPlayerLevel` are immutable once the server is loaded (`sContentPackRegistry->IsFinalized()`). Any changes require a worldserver restart to prevent memory corruption or desynchronized cache states.
- Reloadable settings (`CoAContentScaling.Debug`, `GroupScaling.Enable`, `AdaptiveMechanics.Enable`, `SoloAssist.Mode`, LFG defaults) can be safely modified and reloaded at runtime via `.reload config` without restart via `LoadReloadableConfig()`.

---

## 7. Sign-Off & Release Conclusion

```text
===============================================================
COA UNIVERSAL CONTENT SCALING — RELEASE CANDIDATE READY
ALL PLANNED SCALING ROUNDS CLOSED
===============================================================
```
- **AzerothCore CoA Core Fork**: Clean, built, verified, all tests passing.
- **Module `mod-coa-content-scaling`**: Verified against all progression and encounter test suites.
- **Module `mod-coa-playerbots`**: Untouched and intact.
