# Encounter Adaptation Manifest & Solo/Small-Group Contract

This manifest formalizes the mechanics adaptations for curated raid and dungeon encounters under **CoA Universal Content Scaling (Round 4)**.

---

## 1. Core Principles

1. **Scale Impossibility, Not Identity**: Boss encounters retain their thematic mechanics, timers, and execution identity. Mechanics only adapt when they impose physical multi-player deadlocks (e.g. required simultaneous points, simultaneous vehicle seats, un-dispelled debuffs requiring tank rotation, impossible healing checks in solo non-healer setups).
2. **Full-Group Parity**: When `mechanicParticipants >= intendedPlayers` and no challenge mode downscale is set, all adapters and generic systems **must return the authored values exactly**.
3. **Playerbot Independence**: Playerbots count towards HP and damage scaling (`combatEffectivePlayers`), but **cannot** be assumed to handle special encounter interactions (orb clicking, vehicle piloting, chess possession, precise split positioning).
4. **Snapshot Freezing**: The participant counts (`actualParticipants`, `combatEffectivePlayers`, `mechanicParticipants`, `intendedPlayers`, `challengeSize`, `compositionMode`, `isPhysicallySolo`, `isMechanicSolo`) are frozen at pull in `EncounterScaleSnapshot`. Mid-encounter party changes do not perturb ongoing boss mechanics.
5. **Encounter Adapter Registry**: Adapters are registered in `AdaptiveEncounterMgr` using map-safe keys `{mapId, encounterId}`.

---

## 2. Participant Definitions & Formulas

```text
actualParticipants = physical Player entities on pull (humans + playerbots, no pets/summons)
combatEffectivePlayers = challengeSize > 0 ? challengeSize : actualParticipants
mechanicParticipants = challengeSize > 0 ? min(actualParticipants, challengeSize) : actualParticipants
isPhysicallySolo = actualParticipants <= 1
isMechanicSolo = mechanicParticipants <= 1
```

### Participant Matrix Examples:
- **2 actual + challenge10**: `combatEffective = 10`, `mechanicParticipants = 2` (Boss has 10-man HP/damage, but mechanics require at most 2 players).
- **5 actual + challenge2**: `combatEffective = 2`, `mechanicParticipants = 2` (Boss has 2-man HP/damage, mechanics scale to 2 players).
- **5 actual + adaptive (0)**: `combatEffective = 5`, `mechanicParticipants = 5`.
- **1 actual + adaptive (0)**: `combatEffective = 1`, `mechanicParticipants = 1` (`isPhysicallySolo = true`, `isMechanicSolo = true`).

---

## 3. Curated Encounter Adaptation Matrix

| Encounter | Map ID | Encounter ID | Authored Size | Mechanic Type | Authored Value | Solo / Small-Group Scaled Value | Full-Group Value | Rationale |
|---|---|---|---|---|---|---|---|---|
| **Razorgore the Untamed** | 469 | 0 | 40 | `OBJECTIVE_COUNT` (Eggs) | 30 | 6 (solo), scaled `(mechanics / 40) * 30` | 30 | Prevents solo player wiping to endless add waves during 30 egg channels. |
| **Razorgore the Untamed** | 469 | 0 | 40 | `TIMER_MS` (Exhaustion) | 60000ms | 0ms (solo), 15000ms (2p) | 60000ms | Solo player has no co-charmer to rotate; 60s cooldown is an instant fail. |
| **Twin Emperors** | 531 | 7 | 40 | `PROXIMITY_DISTANCE` | 60y | 0y (solo), 20y (2p) | 60y | Proximity heal heals both bosses every 3.6s if within 60y. Disabling in solo prevents infinite heal deadlock. |
| **Chess Event** | 532 | 9 | 10 | `TIMER_MS` (Cheat Fire) | ~30s | ~45s (solo) | 30s | Allows solo player reasonable reaction window to move pieces while AI plays others. |
| **Four Horsemen** | 533 | 12 | 25 | `REQUIRED_PLAYERS` (Tanks) | 4 | 1 (solo), 2 (2p) | 4 | Group does not need 4 simultaneous players to hold 4 corners. |
| **Four Horsemen** | 533 | 12 | 25 | `TIMER_MS` (Mark Aura) | 12000ms | 36000ms (solo), 24000ms (2p) | 12000ms | Stacking mark interval tripled in solo, allowing sequential corner clearing without lethal damage stacks. |
| **Four Horsemen** | 533 | 12 | 25 | `FAIL_THRESHOLD` (Punish) | 1 (Active) | 0 (Suppressed if `<4` players) | 1 | Idle corner bosses do not wipe the party with global Condemnation / Unyielding Pain spam. |
| **Flame Leviathan** | 603 | 0 | 10/25 | `REQUIRED_PLAYERS` (Overload) | 2 | 1 (solo) | 2 | 1 ejected player can overload circuit on boss back. |
| **Flame Leviathan** | 603 | 0 | 10/25 | `STACK_THRESHOLD` (Speed) | 20 | 10 (solo) | 20 | Gathering Speed buildup slowed/capped so a solo vehicle is not run down instantly. |
| **Valithria Dreamwalker** | 631 | 10 | 10/25 | `REQUIRED_INTERACTORS` | 3 (10m) / 8 (25m) | 1 (solo), `min(players, authored)` | 3 / 8 | Portal spawn count matches available players, preventing missed portal punishment. |
| **Valithria Dreamwalker** | 631 | 10 | 10/25 | `HEALING_CONTRIBUTION` | 0% | 8% per key add kill (solo) | 0% | Enables non-healing classes/builds to complete encounter via add priority kills. |

---

## 4. Generic Core & Script Integration

- **Hook**: `sScriptMgr->OnResolveEncounterMechanic(map, encounterId, mechanicType, authoredValue, resolvedValue)`
- **InstanceScript API**: `uint32 InstanceScript::ResolveEncounterMechanic(uint32 encounterId, uint8 mechanicType, uint32 authoredValue)`
- Core scripts remain completely decoupled from module headers via this standard AzerothCore `AllMapScript` hook mechanism.
- Diagnostics: `.coascale encounter` inspects live encounter state, active locks, participant splits, registered adapter status, and test evaluations.
