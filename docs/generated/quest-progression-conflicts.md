# Quest Progression Conflicts & Reachability Scan

## 1. Scope & Objective

This report evaluates quest progression reachability, chain continuity, and prerequisite consistency across all layout configurations under Universal Content Scaling.

The core requirement:
> No quest chain can become uncompletable or dead-ended due to scaling. If Quest A leads to Quest B (`NextQuestInChain`), the effective level and `MinLevel` of Quest B must be reachable upon completing Quest A.

---

## 2. Methodology & In-Memory Level Resolution

Rather than altering `quest_template` database records, `LocalLevelScaling::QuestMinLevelOwner` dynamically resolves:
1. `GetEffectiveQuestLevel(quest)`: maps `quest_template.QuestLevel` through `ProgressionLayout::MapAuthoredToEffective(era, authoredLevel)`.
2. `GetEffectiveQuestMinLevel(quest)`: maps `quest_template.MinLevel` through `ProgressionLayout::MapAuthoredToEffective(era, authoredMinLevel)`.
3. If an era is disabled, `GetEffectiveQuestMinLevel(quest)` returns `255`, safely hiding the quest from players and questgivers.

---

## 3. Conflict Scan Findings

### 3.1 Prerequisite Level Gaps
- **Authored State**: In Blizzard data, quests typically have `MinLevel` set to $QuestLevel - 5$ or $QuestLevel - 7$.
- **Compressed State (Cap 60, 3 eras)**:
  - Authored span $1..80$ is compressed to $1..60$ ($0.75\times$ scale) or era bands:
    - Classic: $1..60 \to 1..26$ ($0.43\times$)
    - TBC: $58..70 \to 26..43$ ($1.42\times$ relative span)
    - WotLK: $68..80 \to 43..60$ ($1.42\times$ relative span)
  - Due to monotonic mapping:
    $$\forall Q: Authored(MinLevel) \le Authored(QuestLevel) \implies Effective(MinLevel) \le Effective(QuestLevel)$$
  - **Result**: No quest in any era has $Effective(MinLevel) > Effective(QuestLevel)$.

### 3.2 Cross-Era Quest Chains
Certain quest chains cross expansion boundaries (e.g. Hero's Call / Warchief's Command, Karazhan attunement, Wrathgate / Battle for the Undercity):
- **Classic to TBC Hand-off**:
  - Authored: Level 58 breadcrumb into Outland.
  - Cap 60 (All Eras): Level 26 handoff directly connects to TBC start level 26. Zero gap.
  - TBC Disabled: Handoff quest MinLevel maps to 255; questgiver will not offer the breadcrumb.
- **TBC to WotLK Hand-off**:
  - Authored: Level 68 breadcrumb to Borean Tundra / Howling Fjord.
  - Cap 60 (All Eras): Level 43 handoff connects to WotLK start level 43. Zero gap.
  - WotLK Disabled: Handoff quest MinLevel maps to 255; questgiver will not offer the breadcrumb.

### 3.3 Neutral Hub Questgiver Leaks
- **Finding**: High-level questgivers in capital cities (e.g., Archmage Cedric, Ambassador Hellcaller) offering quests for other expansions.
- **Resolution**:
  - Core hook `PlayerScript::OnPlayerCanTakeQuest` checks `sCoAContentScaling->GetLayout().IsEraEnabled(era)`.
  - If the content pack is disabled, the hook returns `false`, preventing pickup even if interacting with the NPC in Stormwind or Orgrimmar.
  - In addition, `GetEffectiveQuestMinLevel` returns 255, completely suppressing the `!` quest icon.

---

## 4. Conflict Scan Summary Table

| Category | Authored Conflict Risk | Scaling Impact | Mitigation Applied | Status |
|---|---|---|---|---|
| Same-Era Chain Gaps | Zero | Preserved monotonic | Proportional mapping | RESOLVED |
| Cross-Era Breadcrumbs | Low | Clean era boundaries | Exact boundary matching | RESOLVED |
| Disabled Pack Questgivers | Medium | Could offer locked quests | Era check in `OnPlayerCanTakeQuest` + MinLevel=255 | RESOLVED |
| Level-Cap Conversions | Medium | Runaway Gold at Cap | Cap 50g reward guard in resolver | RESOLVED |
