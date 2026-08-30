---
name: project-layer-cache-pick
description: "DX12 pick tests the CACHED aabb.layerMask, not the live LayerComponent — transient layer swaps are no-ops (the zoom-fire bug class)"
metadata: 
  node_type: memory
  type: project
  originSessionId: 9a28c586-4c13-4447-916e-7fb51301bfa8
  modified: 2026-08-14T02:33:53.364Z
---

# The cached-layerMask pick class (closed 2026-08-14, game 2.48)

**DX12 `Scene::Intersects` tests `aabb_objects[i].layerMask` — a CACHE rewritten only during
`Scene::Update` (engine `wiScene.cpp:7180` / `:5516`). The DX11 fork's `Pick` read the LIVE
`LayerComponent`.** Any swap-pick-restore done entirely between updates (the pattern
`IntersectAllEx` uses to exclude the player's gun and `pIgnoreObject`,
`CObjectsC_part3.cpp:1780-1786`) was therefore a silent no-op on DX12: the weapon swallowed the
zoomed bullet ray (`Pick` returns only the CLOSEST hit; an out-of-entity-range hit is reported
as a TOTAL MISS, not "hit something undamageable"). Ammo/flash/sound live upstream of the ray,
which made it read as "raycast broken" instead of "raycast eaten".

**Fix (game-side, keep it that way):** `WickedCall_SetObjectRenderLayer` writes the cache
through alongside the live component (`objects.GetIndex(entity)` → `aabb_objects[idx].layerMask`).
Cost at swap time only; next `Scene::Update` re-derives the cache, so no drift.

Why only ZOOM broke: zoom pulls the gun onto the camera axis (`G-Gun_part1.cpp:69`); hip-fire
leaves it offset from the ray. The porters had even noted "this does not work, cannot seem to
set the layermask AFTER you have created the object" — the note WAS this bug.

**Rule:** for any transient state on wicked components, ask *who reads the cache vs the live
component*. The [[project-next-action-immediate]] banner carries the sibling rules from the same
night (modal legacy error paths, buffered timestampactivity).

Related landmines of the same "paired thing, one half dead on DX12" family:
GetImagePointer vs GetImagePointerView (2.37), m_pD3D==NULL blanket guards (2.50 videos),
GetBackBufferForGG NULL stub (2.49 crash), DX11-only dig-a-hole scissors (2.49 preview).
