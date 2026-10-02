# Round 3.3 — Source Graph Authority & Census Completeness Review

## 1. Executive Summary

В раундах Round 3.1 и Round 3.2 была создана и подключена к runtime система Content Census (`GeneratedContentCensus.h` v310, `ItemScalingContext`, `ItemScalingPolicy`). Она устранила слепое использованиеauthored `ItemLevel` в качестве единственного признака тиража и ввела idempotency guard для мутаций предметов в памяти.

Однако независимый аудит выявил следующие незавершенные аспекты перед переходом к Round 4:
1. **Отсутствие графа источников предметов в генераторе переписи**: `source_map` для всех предметов в `generate_census.py` оставался `0`, а тираж предмета определялся пороговыми эвристиками по `ItemLevel` (например, `ilvl >= 152` -> `RAID_PINNACLE`). Из-за этого легендарный клинок Иллидана *Warglaive of Azzinoth* (Item 32837, ilvl 156), добываемый в Black Temple (Map 564, `RAID_END`), ошибочно попадал в `RAID_PINNACLE`.
2. **Дублирование кастомных политик и магические константы**: В `ItemBudgetScaler.cpp` присутствовали жестко закодированные диапазоны ID (`100000`, `200000`, `350000`, `600000`), в то время как авторитетные политики уже определены в `data/content/overrides/custom_content.json`.
3. **Объединение полос DUNGEON_HEROIC и RAID_ENTRY**: В `ItemBudgetScaler.cpp` подземелья героической сложности делили одну полосу силы с рейдами начального уровня (`RAID_ENTRY`), нарушая строгую иерархию `DUNGEON_NORMAL < DUNGEON_HEROIC < RAID_ENTRY < RAID_MID < RAID_END < RAID_PINNACLE`.
4. **Искусственное усечение переписи размещений существ**: `generate_census.py` содержал срез `[:4000]`, индексируя лишь первые 4,000 существ, оставляя существ с высокими ID без информации о карте спавна.

Данный раунд (**Round 3.3**) полностью устраняет эти недостатки, закрепляя граф источников как единственный источник истины.

---

## 2. Issues Audit Matrix

| ISSUE | SEVERITY | CURRENT BEHAVIOR | EXPECTED BEHAVIOR | FIX | TEST | STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Warglaive of Azzinoth misclassified as RAID_PINNACLE** | **CRITICAL** | Item 32837 имеет ilvl 156 -> эвристика назначает `RAID_PINNACLE`; `sourceMap == 0` | Warglaive падает с Иллидана (22917) в Black Temple (Map 564, `RAID_END`). Должен иметь `tier = RAID_END`, `sourceMap = 564` | Построить Item Source Graph в генераторе; привязать добычу к карте и профилю инстанса | `WarglaiveOfAzzinothIsRaidEndNotPinnacle` | **RESOLVED** |
| **Sunwell Plateau items preservation** | **HIGH** | Предметы Sunwell (Map 580) классифицируются только по ilvl | Предметы с Map 580 (Sunwell) авторитетно получают `RAID_PINNACLE` через привязку к инстансу | Маппинг спавнов и боссов Sunwell на Map 580 (`tier = RAID_PINNACLE`) | `SunwellItemsPreserveRaidPinnacle` | **RESOLVED** |
| **WotLK raid tiers driven by ilvl instead of source map** | **HIGH** | Naxx/Ulduar/ToC/ICC разделяются порогами ilvl | Naxx (533) -> `RAID_ENTRY`, Ulduar (603) -> `RAID_MID`, ToC (649) -> `RAID_END`, ICC (631) -> `RAID_PINNACLE` | Авторитетное извлечение карты босса/инстанса через `pve_instance_profiles` | `WotlkRaidTiersDrivenBySourceMap` | **RESOLVED** |
| **Custom policies duplicated across json and C++** | **HIGH** | `ItemBudgetScaler.cpp` содержит хардкод `proto->ItemId >= 100000`, дублируя `custom_content.json` | `custom_content.json` — единый источник истины. Политика экспортируется в `GeneratedItemSourceProfile::policy` | Добавить `policy` в структуру профиля; в C++ убрать жесткие диапазоны ID | `CustomPolicyDrivenByCensusProfile` | **RESOLVED** |
| **DUNGEON_HEROIC and RAID_ENTRY share same power band** | **MEDIUM** | В `ItemBudgetScaler.cpp` тир `DUNGEON_HEROIC` падает в `case ContentTier::RAID_ENTRY` | Строгое разделение: `DUNGEON_NORMAL < DUNGEON_HEROIC < RAID_ENTRY < RAID_MID < RAID_END < RAID_PINNACLE` | Выделить отдельные диапазоны уровней/ilvl для героических подземелий в TBC и WotLK | `DungeonHeroicPowerBandDistinctFromRaidEntry` | **RESOLVED** |
| **Creature census arbitrarily capped at 4000 entries** | **MEDIUM** | `generate_census.py` делает `sample_creatures[:4000]` | 100% заспавненных существ входят в перепись с быстрым $O(\log N)$ поиском | Снять ограничение `[:4000]`, генерировать полный отсортированный массив всех спавнов | `HighEntryCreaturePlacementIndexed` | **RESOLVED** |
| **Diagnostic command missing authority origin** | **LOW** | `.coascale item` не показывает источник авторизации (Loot Map vs Custom Override vs Fallback) | Диагностика отображает `Authority: INSTANCE_LOOT / CUSTOM_OVERRIDE / FALLBACK_ILVL` | Обновить `HandleItem` в `CoAContentScalingCommands.cpp` | Ручной тест `.coascale item` | **RESOLVED** |
