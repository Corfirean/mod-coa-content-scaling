# CoA Universal Content Scaling: Progression Layout Report

## 1. Executive Summary

The Universal Content Scaling Progression Layout defines the authoritative level mapping for all game eras across arbitrary player level caps (e.g. 60, 70, 80) and enabled content pack configurations.

All mapping guarantees:
1. **Monotonicity**: Within any era, lower authored levels map to strictly less than or equal effective levels ($L_1 \le L_2 \implies E(L_1) \le E(L_2)$).
2. **Era Continuity**: Across era boundaries, transitions have 0 gap ($Max(Era_N) = Min(Era_{N+1})$).
3. **Identity Preservation**: Cap 80 with all expansions active retains 1:1 stock Blizzard ranges ($Classic = 1..60, TBC = 58..70, WotLK = 68..80$).
4. **Pack Lockout**: Disabled expansions map to lockout level (255) for all quests, instances, and creatures.

---

## 2. Layout Matrix by Configuration

| Configuration | Max Level | Enabled Packs | Classic Range | TBC Range | WotLK Range | Total Progression Band |
|---|---|---|---|---|---|---|
| **A. Cap 60 (All Eras)** | 60 | Classic, TBC, WotLK | 1 – 26 | 26 – 43 | 43 – 60 | 60 levels (compressed) |
| **B. Cap 60 (Classic Only)** | 60 | Classic | 1 – 60 | [Disabled] | [Disabled] | 60 levels (full authored) |
| **C. Cap 60 (Classic + TBC)** | 60 | Classic, TBC | 1 – 45 | 45 – 60 | [Disabled] | 60 levels (2-era split) |
| **D. Cap 60 (Classic + WotLK)** | 60 | Classic, WotLK | 1 – 45 | [Disabled] | 45 – 60 | 60 levels (skipped era) |
| **E. Cap 70 (All Eras)** | 70 | Classic, TBC, WotLK | 1 – 35 | 35 – 53 | 53 – 70 | 70 levels (balanced) |
| **F. Cap 70 (Classic + TBC)** | 70 | Classic, TBC | 1 – 58 | 58 – 70 | [Disabled] | 70 levels (TBC authentic) |
| **G. Cap 80 (Stock Blizzard)** | 80 | Classic, TBC, WotLK | 1 – 60 | 58 – 70 | 68 – 80 | 80 levels (stock 1:1) |

---

## 3. Tier Unlock Windows by Era & Cap

Tier unlocks stagger content within each era's level band to ensure the natural progression chain:
`DUNGEON_NORMAL < DUNGEON_HEROIC < RAID_ENTRY < RAID_MID < RAID_END < RAID_PINNACLE`

### Cap 60 (All Eras Active: Classic 1-26, TBC 26-43, WotLK 43-60)
- **Classic Tier Windows**:
  - Normal Dungeons: Level 1 – 26
  - Raid Entry (MC, Onyxia): Level 24
  - Raid Mid (BWL, ZG): Level 25
  - Raid End / Pinnacle (AQ40, Naxx): Level 26
- **TBC Tier Windows**:
  - Normal Dungeons: Level 26 – 43
  - Heroic Dungeons: Level 38
  - Raid Entry (Kara, Gruul, Magtheridon): Level 40
  - Raid Mid (SSC, TK): Level 41
  - Raid End / Pinnacle (Hyjal, BT, Sunwell): Level 43
- **WotLK Tier Windows**:
  - Normal Dungeons: Level 43 – 60
  - Heroic Dungeons: Level 55
  - Raid Entry (Naxx80, OS, EoE, Vault): Level 57
  - Raid Mid (Ulduar): Level 58
  - Raid End / Pinnacle (ToC, ICC, RS): Level 60

### Cap 80 (Stock Blizzard Identity)
- **Classic**: Dungeons 1-60, Raids 60
- **TBC**: Normal Dungeons 58-70, Heroic Dungeons 70, Raids 70
- **WotLK**: Normal Dungeons 68-80, Heroic Dungeons 80, Raids 80

---

## 4. Verification & Invariants

1. All layouts pass `ProgressionLayout::Validate()` without errors or gaps.
2. Era overlaps exist only in authentic Blizzard Cap 80 mode (58-60 Classic/TBC overlap, 68-70 TBC/WotLK overlap).
3. Compressed mode (Cap 60 all eras) eliminates overlap to give each era dedicated level progression.
