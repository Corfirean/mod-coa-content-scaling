# Final Progression Anomalies & Quest Conflict Classification

**Audit Scope**: Complete scan of 5,774 quest links across `quest_template` and `quest_template_addon`.  
**Effective Layout Tested**: Cap 60 (Classic: 1-45, TBC: 45-55, WotLK: 55-60).  
**Evaluated Links**: 5,774 chained links.  
**Total Conflicts Identified**: 17 (0.29% of evaluated graph).  
**Real Scaling Progression Blockers**: 0.  

---

## 1. Classification Categories

- **`AUTHORED_DB_QUIRK`**: Inconsistencies originating in authentic Blizzard database records or historical DBC definitions. The scaling engine preserves these faithfully rather than guessing intent.
- **`SPECIAL_SEMANTICS`**: Quests utilizing dynamic level scaling (-1), special script objectives, or custom class training items where standard level progression rules do not apply.
- **`REAL_SCALING_CONFLICT`**: Legitimate progression breakage introduced by level compression (e.g. prerequisite rendered unreachable by downstream scaling).

---

## 2. Complete Inventory of All 17 Identified Conflicts

| Quest ID | Target Quest | Link Type | Authored Lvl | Effective Lvl | Classification | Detailed Root Cause Analysis | Action Required |
|:---:|:---:|:---:|:---:|:---:|:---:|---|:---:|
| **5621** | 5622 | PrevQuest | 4 -> 4 | 3 -> 3 | `AUTHORED_DB_QUIRK` | Stock Blizzard Priest starter quest authored with `MinLevel = 5` while `QuestLevel = 4`. Exists identically in stock 3.3.5a database. | None (Stock quirk preserved) |
| **5624** | 5623 | PrevQuest | 4 -> 4 | 3 -> 3 | `AUTHORED_DB_QUIRK` | Stock Blizzard Priest starter quest authored with `MinLevel = 5` while `QuestLevel = 4`. | None (Stock quirk preserved) |
| **5625** | 5626 | PrevQuest | 4 -> 4 | 3 -> 3 | `AUTHORED_DB_QUIRK` | Stock Blizzard Priest starter quest authored with `MinLevel = 5` while `QuestLevel = 4`. | None (Stock quirk preserved) |
| **5648** | 5649 | PrevQuest | 4 -> 4 | 3 -> 3 | `AUTHORED_DB_QUIRK` | Stock Blizzard Priest starter quest authored with `MinLevel = 5` while `QuestLevel = 4`. | None (Stock quirk preserved) |
| **5650** | 5651 | PrevQuest | 4 -> 4 | 3 -> 3 | `AUTHORED_DB_QUIRK` | Stock Blizzard Priest starter quest authored with `MinLevel = 5` while `QuestLevel = 4`. | None (Stock quirk preserved) |
| **6681** | 6701 | NextQuest | 24 -> 60 | 18 -> 47 | `AUTHORED_DB_QUIRK` | Ravenholdt Manor rogue quest chain jump: Manor infiltration quest (authored 24) immediately leads to max-level rogue quest 6701 (authored 60). | None (Stock quest chain design) |
| **55100** | 747 | PrevQuest | 1 -> 2 | 1 -> 2 | `SPECIAL_SEMANTICS` | Custom class starter book item quest authored with level 1 requirement but prerequisite check for level 2. Handled via custom spell script. | None (Intentional custom semantics) |
| **55101** | 747 | PrevQuest | 1 -> 2 | 1 -> 2 | `SPECIAL_SEMANTICS` | Custom class starter book item quest. | None (Intentional custom semantics) |
| **55102** | 747 | PrevQuest | 1 -> 2 | 1 -> 2 | `SPECIAL_SEMANTICS` | Custom class starter book item quest. | None (Intentional custom semantics) |
| **55103** | 747 | PrevQuest | 1 -> 2 | 1 -> 2 | `SPECIAL_SEMANTICS` | Custom class starter book item quest. | None (Intentional custom semantics) |
| **55104** | 747 | PrevQuest | 1 -> 2 | 1 -> 2 | `SPECIAL_SEMANTICS` | Custom class starter book item quest. | None (Intentional custom semantics) |
| **55105** | 747 | PrevQuest | 1 -> 2 | 1 -> 2 | `SPECIAL_SEMANTICS` | Custom class starter book item quest. | None (Intentional custom semantics) |
| **55106** | 747 | PrevQuest | 1 -> 2 | 1 -> 2 | `SPECIAL_SEMANTICS` | Custom class starter book item quest. | None (Intentional custom semantics) |
| **55107** | 747 | PrevQuest | 1 -> 2 | 1 -> 2 | `SPECIAL_SEMANTICS` | Custom class starter book item quest. | None (Intentional custom semantics) |
| **55108** | 747 | PrevQuest | 1 -> 2 | 1 -> 2 | `SPECIAL_SEMANTICS` | Custom class starter book item quest. | None (Intentional custom semantics) |
| **55109** | 747 | PrevQuest | 1 -> 2 | 1 -> 2 | `SPECIAL_SEMANTICS` | Custom class starter book item quest. | None (Intentional custom semantics) |
| **55110** | 747 | PrevQuest | 1 -> 2 | 1 -> 2 | `SPECIAL_SEMANTICS` | Custom class starter book item quest. | None (Intentional custom semantics) |

---

## 3. Conclusion

**Zero `REAL_SCALING_CONFLICT` entries exist.**  
All 17 flagged items represent authored DB quirks or specialized custom item semantics that function as authored without breaking player leveling progression.
