# Content Census Ambiguities and Resolution Policies

| Ambiguity Category | Target Entity | Context / Root Cause | Authoritative Resolution |
|---|---|---|---|
| REUSED_MAP | Map 249 (Onyxia's Lair) | Re-tuned in 3.3.5 for level 80 10/25 raid | Classified as WotLK RAID_ENTRY with dynamic fallback to Classic 40-man if WotLK disabled |
| REUSED_MAP | Map 533 (Naxxramas) | Level 80 WotLK rework (level 60 version removed) | Classified as WotLK RAID_ENTRY (Cap 80 tuning) |
| PVP_IN_INSTANCE_TPL | Maps 30, 489, 529, 566, 607, 628 | Battlegrounds have instance_template rows | Categorized as MapContentKind::BATTLEGROUND and excluded from PvE scaling |
| DEACTIVATED_LFG_LEGACY | LFGDungeons 1, 2, 14 (WC, Scholo, Gnome) | Set to min=100 max=100 by client developers | Superseded by wing entries (1003-1039), tagged DEACTIVATED_LEGACY |
| HIGH_ENTRY_ITEMS | Entries >= 100000 | 469,388 Ascension cosmetic & trait items | Isolated into CUSTOM_COSMETIC / CUSTOM_CLASS_ITEM and preserved 1:1 |
