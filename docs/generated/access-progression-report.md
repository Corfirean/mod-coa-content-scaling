# Content Access & Unlock Progression Report

## 1. Executive Summary

This report outlines instance, raid, and Dungeon Finder (LFG) access gates under Universal Content Scaling.

Core Objectives:
1. Guarantee that dungeon access follows proper tier sequencing: Normal before Heroic, Heroic before Raid.
2. Ensure LFG queues dynamically reflect the player's scaled level bracket.
3. Completely isolate and lock out instances and queues associated with disabled content packs.

---

## 2. Dungeon & Raid Access Ordering

Instance entry requirements are enforced via `InstanceProfile` and `ProgressionRewardResolver::ResolveTierUnlockLevel`.

### 2.1 Tier Unlock Formula
For an era spanning $[MinLevel, MaxLevel]$ with span $S = MaxLevel - MinLevel$:
- **DUNGEON_NORMAL**: Accessible from $MinLevel$.
- **DUNGEON_HEROIC**: Unlocks at $MinLevel + \lfloor 0.70 \times S \rfloor$.
- **RAID_ENTRY**: Unlocks at $MinLevel + \lfloor 0.80 \times S \rfloor$.
- **RAID_MID**: Unlocks at $MinLevel + \lfloor 0.88 \times S \rfloor$.
- **RAID_END / PINNACLE**: Unlocks at $MaxLevel$.

### 2.2 Concrete Unlock Mapping Examples

#### Layout 1: Cap 60 (All Eras: Classic 1-26, TBC 26-43, WotLK 43-60)
- **Classic**:
  - Dungeons: 1 – 26
  - Raids (MC, Ony, BWL, AQ40, Naxx): 24 – 26
- **TBC**:
  - Normal Dungeons: 26 – 43
  - Heroic Dungeons: 38+
  - Raid Entry (Karazhan, Gruul, Magtheridon): 40+
  - Raid Mid (SSC, TK): 41+
  - Raid End/Pinnacle (Hyjal, BT, Sunwell): 43
- **WotLK**:
  - Normal Dungeons: 43 – 60
  - Heroic Dungeons: 55+
  - Raid Entry (Naxx80, OS, EoE, VoA): 57+
  - Raid Mid (Ulduar): 58+
  - Raid End/Pinnacle (ToC, ICC, RS): 60

#### Layout 2: Cap 60 (Classic Only: 1-60)
- **Classic Dungeons**: 1 – 60
- **Classic Raids**: 58 – 60
- **TBC / WotLK Dungeons & Raids**: Completely disabled and inaccessible (teleport/portal checks reject entry).

#### Layout 3: Cap 80 (Stock Blizzard Identity)
- **Classic**: Dungeons 1-60, Raids 60
- **TBC**: Normal Dungeons 58-70, Heroic Dungeons 70, Raids 70
- **WotLK**: Normal Dungeons 68-80, Heroic Dungeons 80, Raids 80

---

## 3. LFG (Dungeon Finder) Progression Integration

LFG integration matches player queues to effective content ranges:
1. **Dungeon Visibility**: `LFGDungeonEntry` min/max levels are dynamically evaluated against `InstanceProfile::effectiveMinLevel` and `InstanceProfile::effectiveMaxLevel`.
2. **Expansion Filtering**:
   - If TBC is disabled, all TBC LFG entries are excluded from `GetDungeonsForPlayer`.
   - If WotLK is disabled, all WotLK LFG entries are excluded.
3. **Queue Pairing**: Players only match with dungeons whose effective level range encompasses their current character level.

---

## 4. Verification Checkpoints

- [x] Heroic unlock level is strictly greater than Normal entry level for all multi-tier eras.
- [x] Raid Entry unlock level is strictly greater than or equal to Heroic unlock level.
- [x] Raid End / Pinnacle unlock level equals the era's maximum level cap.
- [x] Disabled content pack dungeons return level 255 / inaccessible and are invisible in LFG.
