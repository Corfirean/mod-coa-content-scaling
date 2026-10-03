---
area: scaling
type: fixed
audience: admins
title: Destroyed instances release their saved scaling state
---
Encounter snapshots and group scaling settings are cleared when a map is destroyed, preventing stale state from reaching a later instance that reuses its ID.
