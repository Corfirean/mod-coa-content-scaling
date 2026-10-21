---
area: scaling
type: added
audience: admins
title: Health settings for scaled creatures by place and by role
---
Five new settings, `CoAContentScaling.Difficulty.Health.WorldMultiplier`, `DungeonMultiplier`, `RaidMultiplier`, `TrashMultiplier` and `BossMultiplier`, multiply the health of scaled creatures on top of the existing scaling and the `CoA.CreatureScaling.*.Health` values. All default to 1.0 (nothing changes), range 0.1 to 10.0, and reload with `.reload config`. A dungeon boss gets the dungeon and the boss multiplier.
