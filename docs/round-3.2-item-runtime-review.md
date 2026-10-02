# Round 3.2 — Item Runtime Authority & Final Validation Review

## 1. Executive Summary

В раунде Round 3.1 был построен полный генератор переписи контента (Content Census), создавший `GeneratedContentCensus.h` v310 со статическими профилями экипировки `GeneratedItemSourceProfile` (3,857 записей), определивший эры, тиражи (`ContentTier`), карты происхождения и флаги спец-свойств (`specialFlags`).

Однако текущий runtime `ItemBudgetScaler` страдает от архитектурного разрыва: он не использует сгенерированный `ContentTier` и спец-флаги, а полагается исключительно на эвристические диапазоны authored `ItemLevel` (например, `ilvl <= 226` -> T7, `ilvl <= 252` -> T8). В результате:
1. Предметы с аномальным `ItemLevel` получают неверный тираж вместо авторитетного тиража подземелья/рейда из переписи.
2. Флаги кастомных предметов и особых эффектов (`CUSTOM_COSMETIC`, `CUSTOM_CLASS_ITEM`, `CUSTOM_GAMEPLAY`) не применяются на уровне индивидуальной политики масштабирования.
3. Отсутствует защита от повторного вызова `ScaleAllItems()` (идемпотентность).
4. `OnResolveDungeonAccessLevels` в ядре получает только `mapId`, но `Player::Satisfy` в ядре уже оперирует конкретным выбранным объектом `DungeonProgressionRequirements`, привязанным к целевой сложности в `MapMgr::CanEnter` и `MiscHandler`.

Данный документ фиксирует проблемы, дефекты и план исправления для Round 3.2.

---

## 2. Issues Audit Matrix

| ISSUE | SEVERITY | CURRENT BEHAVIOR | EXPECTED BEHAVIOR | FIX | TEST | STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Generated item tier not used by ItemBudgetScaler** | **CRITICAL** | `ItemBudgetScaler::CalculateItemBudget` масштабирует `ItemLevel` по жестко закодированным границам `proto->ItemLevel` (188-226, 227-252, etc.) | Авторитетный `GeneratedItemSourceProfile::tier` управляет целевой полосой силы предмета; эвристика ilvl — только fallback | Создать `ItemScalingContext` и `CalculateItemBudget(proto, layout, ctx)`, выбирающий тир по авторитетному профилю переписи | `GeneratedItemTierWinsOverMisleadingIlvl` | **RESOLVED** |
| **Generated specialFlags ignored** | **HIGH** | `GeneratedItemSourceProfile::specialFlags` присутствуют в переписи, но runtime их не читает | Четко специфицированный контракт `GeneratedItemSpecialFlags` (Proc, Use, Set, Socket, Custom, Preserve) | Определить битовую маску в `ContentEra.h`/`GeneratedContentCensus.h`, парсить в `ItemScalingContext` | `SpecialFlagsVisibleAndContractEnforced` | **RESOLVED** |
| **Custom item policies not enforced** | **HIGH** | Предметы с `entry >= 100000` масштабируются или игнорируются случайно | `CUSTOM_COSMETIC` и `CUSTOM_CLASS_ITEM` строго получают политику `PRESERVE`, `CUSTOM_GAMEPLAY` — `TIER_ALIGNED` | Реализовать `ItemScalingPolicy`, гарантирующий неизменность полей для `PRESERVE` | `CustomCosmeticAndClassItemPreserved` | **RESOLVED** |
| **ilvl heuristic still primary source of item tier** | **HIGH** | Полоса силы вычисляется исключительно из authored `ItemLevel` | Приоритет: `manual override > generated profile tier > source map profile > legacy ilvl heuristic` | Централизовать выбор тира в `ItemScalingContext::Resolve` | `MissingGeneratedProfileSafelyFallsBack` | **RESOLVED** |
| **ScaleAllItems lack of idempotency protection** | **MEDIUM** | `ScaleAllItems` мутирует память `ItemTemplateStore` напрямую; повторный запуск перемножит статы второй раз | Однократный guard-флаг `_itemsScaled` предотвращает повторную мутацию памяти на reload | Добавить `bool _itemsScaled` в `ItemBudgetScaler` | `ScaleAllItemsDoubleInvocationIsIdempotent` | **RESOLVED** |
| **Dungeon access hook lacks difficulty context in caller** | **MEDIUM** | `Player::Satisfy` вызывается для `(ar, target_map)`. Внутри `Satisfy` сложность не передавалась | Конкретный `ar` уже является вариантом сложности (`GetAccessRequirement(map, difficulty)`), но хук получает `target_map` без `difficulty` | Документировать контракт: `ar` авторитетен для базовых уровней, а `OnResolveDungeonAccessLevels` масштабирует фактические переданные `minLevel`/`maxLevel` через `ProgressionLayout`, не подменяя их | `ReusedMapAccessScalingProof` | **RESOLVED** |
| **Runtime diagnostics missing item authority info** | **LOW** | `.coascale item` выводит только `effectiveReqLevel`, `effectiveItemLevel` и множители | Диагностика отображает `Generated Profile: yes/no`, `Era`, `Tier`, `Source Map`, `Policy`, `Special Flags`, `Fallback: yes/no` | Обновить `HandleItem` в `CoAContentScalingCommands.cpp` | Ручной тест `.coascale item` | **RESOLVED** |
| **Full unit test suite regression coverage** | **HIGH** | Тесты запускались только через фильтр | Полный запуск `unit_tests.exe` без фильтров должен давать 0 FAILED | Запуск всего набора unit-тестов AzerothCore | `FullUnitTestsSuiteAllPass` | **RESOLVED** |

---

## 3. Архитектурная модель Round 3.2

### 3.1. ItemScalingPolicy
```cpp
enum class ItemScalingPolicy : uint8
{
    STANDARD       = 0, // Стандартное масштабирование экипировки
    TIER_ALIGNED   = 1, // Масштабирование строго по полосе ContentTier из переписи
    PRESERVE       = 2, // Полная неприкосновенность боевых характеристик и метаданных
    REVIEW_SPECIAL = 3  // Масштабирование базовых статов с сохранением эффектов proc/use/socket
};
```

### 3.2. GeneratedItemSpecialFlags
```cpp
enum GeneratedItemSpecialFlags : uint8
{
    ITEM_SPECIAL_NONE     = 0,
    ITEM_SPECIAL_PROC     = 1 << 0, // tr1/tr2 in (1, 2)
    ITEM_SPECIAL_USE      = 1 << 1, // tr1/tr2 == 0 with sp > 0
    ITEM_SPECIAL_SET      = 1 << 2, // ItemSet > 0
    ITEM_SPECIAL_SOCKET   = 1 << 3, // Sockets present
    ITEM_SPECIAL_CUSTOM   = 1 << 4, // Custom entry (>= 100000)
    ITEM_SPECIAL_PRESERVE = 1 << 5  // Cosmetic / Class unlocker (no combat stats)
};
```

### 3.3. ItemScalingContext
Структура контекста объединяет авторитетную эру, тир, статус переписи, источник и политику:
```cpp
struct ItemScalingContext
{
    ContentEra era{ContentEra::Classic};
    ContentTier tier{ContentTier::WORLD};
    ItemScalingPolicy policy{ItemScalingPolicy::STANDARD};
    uint32 sourceMap{0};
    uint8 specialFlags{0};
    bool hasGeneratedProfile{false};
    bool fallbackTierInference{false};

    static ItemScalingContext Resolve(ItemTemplate const* proto);
};
```
