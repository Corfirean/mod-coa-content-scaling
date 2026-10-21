# Release Notes: CoA Universal Content Scaling

**Release Version**: 1.0.0-rc1 (Release Candidate 1)  
**Date**: October 2, 2026  
**Status**: Production Ready — All Scaling Rounds 1–5 Closed  

---

## 1. Overview

`mod-coa-content-scaling` delivers a non-destructive, high-performance Universal Content Scaling architecture for AzerothCore and Conquest of Azeroth.

The module allows servers running custom level caps (including Cap 60, Cap 70, Cap 80, or any intermediate level cap) to provide continuous, fair, and engaging progression across Classic, The Burning Crusade, and Wrath of the Lich King content without modifying a single row in the database.

---

## 2. Core Capabilities & Highlights

- **ProgressionLayout System**:
  - Automatically calculates seamless level bands for Classic, TBC, and WotLK based on `CONFIG_MAX_PLAYER_LEVEL`.
  - True Blizzard stock identity when Cap 80: Classic (1–60), TBC (58–70), WotLK (68–80).
  - Smooth monotonic compression when Cap 60: Classic (1–45), TBC (45–55), WotLK (55–60).
  - Strictly independent expansion content packs (`mod-coa-tbc-content`, `mod-coa-wotlk-content`).
- **Combat Budget Normalization**:
  - Completely decouples character combat levels from expansion stat scaling.
  - Normalizes creature health, damage, armor, and mana dynamically based on `ContentTier` (`WORLD`, `DUNGEON_NORMAL`, `DUNGEON_HEROIC`, `RAID_ENTRY`..`RAID_PINNACLE`).
- **1..N Group & Instance Scaling**:
  - Dungeons and raids scale fluidly from solo players to full raid teams using power curves.
  - Anti-exploit combat locks prevent mid-combat scaling manipulation.
- **Adaptive Encounter Mechanics & Solo Assist**:
  - Adapts lethal multi-person mechanics (Razorgore mind controls, Twin Emperors proximity heals, Four Horsemen marks, Flame Leviathan overloads, Valithria healing requirements, Lich King Valkyr drop mechanics).
  - Safe cliff-drop release points ensure solo players are never instantly killed.
- **In-Memory Item Budget Scaling**:
  - Normalizes equipment item levels, required levels, stats, armor, and weapon DPS in memory.
  - Client tooltips reflect scaled stats natively via `CMSG_ITEM_QUERY_SINGLE`.
- **Dungeon Access & LFG Integration**:
  - Difficulty-aware dungeon access requirements.
  - Full support for Matchmaking, Bot-Fill (`mod-coa-playerbots`), and Partial Party queues.
  - Corpse re-entry safety guarantees dead players are never locked out of their instance.
  - Daily random dungeon rewards accurately map to expansion reward tiers.

---

## 3. Administration & Diagnostics

- `.coascale status`: Overall engine state and active flags.
- `.coascale layout`: Progression layout and level boundaries.
- `.coascale validate`: Comprehensive runtime verification check.
- `.coascale census`: Profile counts and integrity stats.
- `.coascale creature <entry>`: Creature stat and tier budget inspection.
- `.coascale quest <questId>`: Quest effective level and era analysis.
- `.coascale item <itemId>`: Item budget, power band, and multiplier inspect.
- `.coascale instance`: Instance scale context and combat lock state.
- `.lfgmode [matchmaking|bots|party]`: Player LFG queue composition.
- `.lfgchallenge [adaptive|1..40]`: Simulated group challenge size.
