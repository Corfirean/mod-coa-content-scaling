# Progression Reward Budget Report

## 1. Executive Summary

This report analyzes XP, Gold, and Item Stat budgets across the progression lifecycle under Universal Content Scaling.

The primary balancing requirements:
1. **Pacing Balance**: Players must not experience instant level-ups from single high-authored quests in compressed eras, nor hit XP droughts.
2. **Economy Protection**: Max-level XP to gold conversion must not cause runaway currency inflation.
3. **Item Power Harmony**: Item required levels must match content acquisition levels, preventing players from being awarded unusable gear or overpoweringly high-stat items.

---

## 2. XP Calibration Mechanics

XP scaling is governed by `ProgressionRewardResolver::ResolveQuestXP`:
- **Identity Mode (Cap 80, all packs enabled)**:
  $$XP_{effective} = XP_{authored} \quad (1:1 \text{ stock Blizzard})$$
- **Compressed Mode (e.g. Cap 60)**:
  $$XP_{effective} = XP_{authored} \times \left(\frac{L_{eff}}{L_{auth}}\right) \times \text{EraSpanFactor}$$
  Where $\text{EraSpanFactor} \in [0.90, 1.15]$ smooths early-to-late era quest rewards.

### Comparison Table: Sample Quest XP

| Quest Sample | Authored Level | Authored XP | Cap 80 XP (All Eras) | Cap 60 XP (All Eras) | Effective Level (Cap 60) |
|---|---|---|---|---|---|
| Hogger (Classic Elwynn) | 11 | 900 | 900 | 450 | 5 |
| Morbent Fel (Classic Duskwood) | 32 | 3,300 | 3,300 | 1,510 | 14 |
| In Dreams (Classic EPL) | 60 | 9,950 | 9,950 | 4,890 | 26 |
| Hellfire Citadel Journey (TBC) | 61 | 10,750 | 10,750 | 5,160 | 27 |
| Akama's Promise (TBC Shadowmoon) | 70 | 19,000 | 19,000 | 13,440 | 43 |
| The Prophet of Sseratus (WotLK) | 75 | 21,150 | 21,150 | 16,800 | 53 |
| All Will Be Well (WotLK Icecrown) | 80 | 27,550 | 27,550 | 23,780 | 60 |

**Observations**:
- Quests scale smoothly in proportion to the era's compressed level bracket.
- Completing a compressed level 43 quest (authored 70) yields XP calibrated to a level 43 character's XP-to-next-level requirement, ensuring healthy leveling pace.

---

## 3. Economy & Gold Conversion Protection

At level cap, unfinished quests convert unneeded XP into copper at the rate of:
$$\text{Copper} = XP \times 6.0 \times \text{BonusMoneyRate}$$

### Safeguard Guardrail
- Under unconstrained conditions, an authored level 80 daily yielding 27,550 XP would convert to $\sim 16.5$ Gold.
- If multiple chain quests or custom quests with oversized XP budgets exist, conversion could yield disproportionate wealth at lower caps.
- `ProgressionRewardResolver::ResolveMoneyAtCap` enforces:
  $$\text{Cap Per Quest} = 500,000 \text{ copper} = 50 \text{ Gold}$$
- Authored gold rewards from `quest_template.RewOrReqMoney` are strictly preserved without inflation.

---

## 4. Item Required Level & Stat Budgets

Item scaling ensures:
1. `budget.effectiveRequiredLevel` does not exceed `effectiveContentLevel + 1`.
2. Item stats (Strength, Agility, Stamina, Spell Power, Weapon DPS, Armor) scale strictly according to `ItemBudgetScaler` power curves derived from the item's census profile.
3. Item tier progression preserves the hierarchy:
   $$\text{DUNGEON\_NORMAL} < \text{DUNGEON\_HEROIC} < \text{RAID\_ENTRY} < \text{RAID\_MID} < \text{RAID\_END} < \text{RAID\_PINNACLE}$$
