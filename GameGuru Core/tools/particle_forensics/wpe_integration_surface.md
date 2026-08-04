# WPE (.PE) Particle System — DX12 Integration Surface

Repo: `D:\max\GameGuruMAXDX12` (HEAD `65a614d2`, branch `main`, worktree `determined-chebyshev-bf0892`).
DX11 reference (READ ONLY): `D:\max\GameGuruMAX`, engine fork `D:\max\WickedRepo`.
DX12 engine: `D:\max\WickedEngineDX12`.

**Scope:** the WPE system = `.pe` files that are Wicked Engine **scene archives** carrying
`wi::EmittedParticleSystem` (`wiEmittedParticle`) components. NOT the `gpup_`/`.arx`
render-to-texture system, NOT `M-Particles.cpp` (`ravey_particles_*` / `CParticleC`),
NOT the DarkSDK decal blitter.

---

## 0. HEADLINE: none of the 14 shipped `.PE` files can load on DX12 today

Every `.pe` in the build area is Wicked archive **version 5076**. Verified by reading the
first 8 bytes (LE uint64) of all 14 files:

```
Explosion Burst.pe 5076   Steam.pe 5076   birds.pe 5076   burst-explosion2-g.pe 5076
dirt-splash4.pe 5076      downpour.pe 5076  dust devil.pe 5076   firearea.pe 5076
heavy-rain3.pe 5076       impact_blood_1.pe 5076   impact_stone_2.pe 5076
impact_water_1.pe 5076    toxic.pe 5076     waterleak.pe 5076
```

- DX11-era fork: `D:\max\WickedRepo\WickedEngine\wiArchive.cpp:7` → `uint64_t __archiveVersion = 5077;`
- DX12 upstream: `D:\max\WickedEngineDX12\WickedEngine\wiArchive.cpp:17` → `static constexpr uint64_t __archiveVersion = 93;`

`Archive::Archive()` (`wiArchive.cpp:76-82`) would pop a modal message box for
`header.version > __archiveVersion`. GG pre-empts that with a **silent skip** in
`WickedCall_LoadWiSceneDirect`:

`GameGuru Core\Guru-WickedMAX\wickedcalls_part3.cpp:1961-1979`
```cpp
	// Validate file is a genuine Wicked Engine archive before opening
	// (old .pe particle files from the DX11 engine have incompatible binary format)
	{
		FILE* fcheck = fopen(filename, "rb");
		if (fcheck)
		{
			uint64_t fileVersion = 0;
			size_t bytesRead = fread(&fileVersion, 1, sizeof(fileVersion), fcheck);
			fclose(fcheck);
			if (bytesRead < sizeof(fileVersion) || fileVersion < 22 || fileVersion > 200)
			{
				char msg[1024];
				sprintf(msg, "Skipped non-Wicked archive: %s (version=%llu, expected 22-93)", filename, (unsigned long long)fileVersion);
				void timestampactivity(int i, char* desc_s);
				timestampactivity(0, msg);
				return 0;
			}
		}
	}
```

Consequence: `WickedCall_LoadWiSceneDirect` returns 0 → `scene.emitters.GetCount()` is
unchanged → **every** downstream `count_before != count_after` test fails → `root` stays 0 →
no emitter, no transform, no action. The C++ plumbing is otherwise intact and *not* stubbed;
it is starved of data at the file gate. **A format migration (or a version-remap shim in
`wiArchive`) is the hard prerequisite for any WPE work.**

---

## 1. Call graph — the WPE API

### 1.1 The layer itself

| Symbol | Definition | Declared |
|---|---|---|
| `WickedCall_LoadWiSceneDirect(Scene&, char* filename, bool attached, char* changename, char* changenameto)` | `Guru-WickedMAX\wickedcalls_part3.cpp:1951` | (local fwd-decls) |
| `WickedCall_LoadWiScene(char*, bool, char*, char*)` | `wickedcalls_part3.cpp:2118` | `wickedcalls.h:253` |
| `WickedCall_LoadWPE(char* filename)` | `wickedcalls_part3.cpp:2130` | `wickedcalls.h:257` |
| `WickedCall_PerformEmitterAction(int iAction, uint32_t emitter_root)` | `wickedcalls_part4.cpp:3` | `wickedcalls.h:255` |
| `WickedCall_ParticleEffectPositionRotation(uint32_t root, float x,y,z, xa,ya,za)` | `wickedcalls_part4.cpp:66` | (local fwd-decls only) |
| `WickedCall_ParticleEffectPosition(uint32_t root, float x,y,z)` | `wickedcalls_part4.cpp:87` | (local fwd-decls only) |
| `WickedCall_UpdateEmitters(void)` | `wickedcalls_part4.cpp:102` | `wickedcalls.h:256` |
| `WickedCall_CreateEmitter(std::string& name, float x,y,z, uint32_t proot)` | `wickedcalls_part4.cpp:204` | `wickedcalls.h:258` |
| `GetVisibleWEmitters(void)` | `wickedcalls_part4.cpp:295` | **nowhere** |
| `DeleteEmitterEffects(uint32_t root)` | `DarkLUA\DarkLUA_part5.cpp:998` | local `void DeleteEmitterEffects(uint32_t root);` at each call site |
| `CleanUpEmitterEffects(void)` | `DarkLUA\DarkLUA_part5.cpp:1023` | local fwd-decl |

Gating: the whole block is inside `#ifdef WICKEDPARTICLESYSTEM`
(`wickedcalls_part3.cpp:1950` … `:2162` "continued in wickedcalls_part4.cpp",
`wickedcalls_part4.cpp:1` … `:312`). The macro **IS** defined unconditionally at
`GameGuru\Include\preprocessor-moreflags.h:119` → `#define WICKEDPARTICLESYSTEM`.
It is the only definition in the tree; the system is compiled in.

`iAction` codes (`wickedcalls_part4.cpp:2`):
```
1 Burst all. 2 = Pause. - 3 = Resume. - 4 = Restart - 5 - visible - 6 = not visible. - 7 = pause emit - 8 = resume emit
```
**Actions 5, 6, 7, 8 are dead** (`wickedcalls_part4.cpp:40-59`): the DX12 upstream
`EmittedParticleSystem` has no `SetVisible`/`SetEmitPaused`. All four cases are empty:
```cpp
					case 5:
					{
						//ec->SetVisible(true); // SetVisible not in EmittedParticleSystem
						break;
					}
					...
					case 7:
					{
						//ec->SetEmitPaused(true); // SetEmitPaused not in EmittedParticleSystem
						break;
					}
```
DX11 fork had them: `D:\max\WickedRepo\WickedEngine\wiEmittedParticle.h:168-169`
(`IsVisible`/`SetVisible`), `:172` (`IsActive`), `:205` (`SetEmitPaused` with `FLAG_EMIT_PAUSE`).
DX12 `wiEmittedParticle.h` has only `SetPaused` (:161), `Burst` (:72-74), `Restart` (:75),
`IsInactive` (:143). So **only actions 1, 2, 3, 4 do anything.**

### 1.2 `WickedCall_LoadWPE` — 3 real call sites

| file:line | enclosing | purpose |
|---|---|---|
| `GameGuru\Imgui\imgui_gg_dx11_part5.cpp:249` | property-panel `wpefile` widget, on value change | reload the editor preview emitter when the user picks a different `.pe` |
| `imgui_gg_dx11_part5.cpp:265` | same widget, `Preview` checkbox turned on | create the editor preview emitter |
| `GameGuru\Source\M-Weapon.cpp:1215` | `weapon_projectile_*` creation (per projectile instance) | attach a `.pe` trail to a projectile: `t.WeaponProjectile[t.tNew].WPE_Root = WickedCall_LoadWPE(t.WeaponProjectileBase[t.tNewProjBase].WPE_Effect.Get());` then `WickedCall_PerformEmitterAction(6, ...)` (dead action) to hide it |
| `M-Weapon.cpp:1441` | — | commented-out fwd-decl, not a call |

`WickedCall_LoadWPE` (`wickedcalls_part3.cpp:2130`) does `GG_GetRealPath(path, 0)` →
`WickedCall_LoadWiScene` → picks the **last** emitter in the scene, walks
`hierarchy.GetComponent(emitter)->parentID` to get `root`, calls `ec->Restart()`, returns
`root`. The `//ec->SetVisible(false); // SetVisible removed from EmittedParticleSystem`
line is dead, so a loaded preview/trail is **immediately visible** whether or not the caller
wanted it hidden.

### 1.3 `WickedCall_PerformEmitterAction` — 20 call sites

**Decal / preload path** (`GameGuru\Source\M-Entity_part5.cpp`):
- `:121` action 2 (Pause), `:122` action 6 (Not Visible) — in `preload_wicked_particle_effect`, the **reject** branch when the loaded `.pe` turns out not to be burst-only (`ec->count > 0.1f`, `:115`).
- `:132`/`:133` actions 2 + 6 — normal end of preload: cache the emitter, park it paused+invisible.
- `:253`-`:256` actions 3 (Resume), 4 (Restart), 5 (Visible), 1 (Burst All) — `newparticle_updateparticleemitter`, the **only** WPE runtime "fire" path, gated on `pParticle->bParticle_Fire == true`.

**Editor preview lifecycle** (all `action 6` = the dead "not visible", then `DeleteEmitterEffects`):
- `Imgui\imgui_gg_dx11_part5.cpp:245` (effect file changed), `:251-253` (1/4/5 after reload), `:268-270` (1/4/5 on Preview tick), `:275` (Preview untick), `:284` ("Burst" button → action 1).
- `Source\M-GridEditB_part12.cpp:179` (character panel, script changed), `:1824` (particle marker panel exit), `:2228` (object panel, script changed), `:2450` (generic `ismarker > 1` panel, script changed).
- `Source\M-GridEditB_part24.cpp:1008` — `RenderPreviewEmitter()`, selection moved to a different entity.

**Weapon / projectile** (`GameGuru\Source\M-Weapon.cpp`):
- `:1217` action 6 right after `WickedCall_LoadWPE` — intended "start hidden", currently a no-op.
- `:1370-1372` actions 4 (Restart), 5 (Visible), 8 (Resume emit) — projectile fired.
- `:1693` action 7 (Pause emit) — projectile destroyed; comment `//PE: Stop emit new particles. but keep simulation active.` **Dead** → projectile trails never stop emitting.

**LUA**: `DarkLUA\DarkLUA_part5.cpp:1209` inside `WParticleEffectAction`.

### 1.4 `WickedCall_ParticleEffectPosition` / `...PositionRotation`

| file:line | enclosing | purpose |
|---|---|---|
| `M-Weapon.cpp:391` | projectile move, non-physics branch | track the `.pe` trail to the projectile (`+addy` 2 or 5) |
| `M-Weapon.cpp:411` | projectile move, physics branch | same |
| `M-Weapon.cpp:1367` | projectile launch | snap trail to muzzle start position |
| `M-GridEditB_part24.cpp:1023` | `RenderPreviewEmitter()` | `WickedCall_ParticleEffectPositionRotation(PreviewWPERoot, posx, posy + fPreviewYOffset, posz, 0, posya, 0)` — pin the editor preview to the selected entity, Y-offset from the script's `offsety` property |

Both are trivial `TransformComponent::ClearTransform / RotateRollPitchYaw / Translate /
UpdateTransform` on the **root** entity — they do not touch the emitter components.

### 1.5 `WickedCall_UpdateEmitters` — 1 call site, and it is a no-op

`Guru-WickedMAX\master_part1.cpp:421-425` (inside `Master::Update`, after terrain/trees/grass,
before `GuruLoopRender()`):
```cpp
#ifdef WICKEDPARTICLESYSTEM
		auto range4 = wiProfiler::BeginRangeCPU("Update - Emitters");
		WickedCall_UpdateEmitters();
		wiProfiler::EndRange(range4);
#endif
```
The body (`wickedcalls_part4.cpp:102-202`) iterates `scene.emitters` and then hits:
```cpp
		//PE: If bFollowCamera , find InDoor , OutDoor , UnderWater.
		//PE: bFindFloor ONLY if ec->bFollowCamera
		//if (ec && (ec->bFindFloor || ec->bFollowCamera)) // bFindFloor/bFollowCamera not in EmittedParticleSystem
		if (false) // disabled: bFindFloor/bFollowCamera not in EmittedParticleSystem
```
Everything inside is `if (false)`. The `#ifdef WPEDebug` box-draw block is also compiled out
(`:101` `//#define WPEDebug`). **Net: the per-frame emitter update does nothing but walk the
emitter list.** DX11 fork had the members: `WickedRepo\WickedEngine\wiEmittedParticle.h:143`
`bool bFindFloor = false;` and `:146` `bool bFollowCamera = false;` — these were GG-fork
additions to the engine, removed by the DX12 upstream rebase. Weather/rain `.pe` effects that
follow the camera and snap to the floor are therefore **feature-lost**, not merely stubbed.

### 1.6 `WickedCall_CreateEmitter` and `GetVisibleWEmitters` — dead code

- `WickedCall_CreateEmitter` (`wickedcalls_part4.cpp:204`): **zero callers**. Also buggy —
  `Scene scene;` at `:209` shadows the global scene with a stack-local, and
  `XMMATRIX& transformMatrix = XMMatrixIdentity();` at `:210` binds a non-const ref to a
  temporary. It was presumably a scratch prototype for authoring an emitter from code.
- `GetVisibleWEmitters` (`wickedcalls_part4.cpp:295`): **zero callers, no declaration**. Its
  two filters are commented out (`//if (!ec.IsVisible())`, `//if (!ec.IsActive())`), so it
  would return the raw emitter count anyway.

### 1.7 Lifecycle helpers

`DeleteEmitterEffects(root)` (`DarkLUA_part5.cpp:998`) — collects every emitter whose
`hierarchy.parentID == root`, plus root itself, and `scene.Entity_Remove`s them.
17 call sites: `imgui_gg_dx11_part5.cpp:247/277`; `M-Entity_part4.cpp:1888` (decal cache
teardown) and `:1910` (live decal-element teardown); `M-Entity_part5.cpp:124` (reject a
non-burst decal emitter); `M-GridEditB_part12.cpp:181/1826/2230/2452`;
`M-GridEditB_part24.cpp:1010`; `M-GridEdit_part0.cpp:1645`; `M-GridEdit_part1.cpp:383/441`.

`CleanUpEmitterEffects()` (`DarkLUA_part5.cpp:1023`) — drains `vWickedEmitterEffects`
(the roots pushed by `WParticleEffectLoad`, `:1104`). Called at
`M-Game_part2.cpp:144-145` and `M-GridEdit_part2.cpp:1312-1313`, both with the comment
`//PE: Clear all wicked particle effects created by lua.` — i.e. on test-game exit.

`delete_notused_decal_particles()` (`M-Entity_part4.cpp:1878`) — the shared teardown: WPE
decal instances go through `DeleteEmitterEffects` (`:1903-1912`, gated on `bWPE`), legacy
ones through `gpup_deleteEffect(delete_decal_particles[i])` (`:1918`). **This function is the
one place both systems are torn down side by side.**

`RenderPreviewEmitter()` (`M-GridEditB_part24.cpp:995`) — called once per editor frame from
`M-GridEdit_part0.cpp:1578-1579`.

---

## 2. The `bWPE` boundary — where the line is drawn

`newparticletype::bWPE` — declared `GameGuru\Include\Types.h:5493`, defaulted `false` at
`Types.h:5541`.

### 2.1 `newparticletype` is embedded in exactly three structs

| Types.h | struct | meaning |
|---|---|---|
| `:6300` | `entityeleproftype` (opens `:6252`) | per-placed-entity properties (the thing that lives in `t.entityelement[e].eleprof` and `t.grideleprof`) |
| `:7903` | `decaltype` (opens `:7896`) | a decal **bank definition** loaded from `gamecore\decals\<name>\` |
| `:7948` | `decalelementtype` (opens `:7947`) | a live, reusable decal **instance** |

### 2.2 `bWPE` is only ever WRITTEN in `M-Decal.cpp`

```
M-Decal.cpp:221   t.decal[t.decalid].newparticle.bWPE = false;   // decal_load(): default
M-Decal.cpp:253   t.decal[t.decalid].newparticle.bWPE = true;    // wpe.pe exists AND full-effect loading enabled
M-Decal.cpp:262   t.decal[t.decalid].newparticle.bWPE = false;   // preload rejected the effect (not burst-only)
```
`decal_load()` (`M-Decal.cpp:209`):
```cpp
	t.decal[t.decalid].newparticle.bWPE = false;
	t.strwork = ""; t.strwork = t.strwork + "gamecore\\decals\\" + t.decal_s + "\\wpe.pe";
	t.decal[t.decalid].newparticle.emitterid = -1;
	t.decal[t.decalid].newparticle.emittername = t.strwork.Get();
	...
	if (!bNotInStandalone && FileExist(pAbsPathToParticle) == 1)
	{
		extern bool g_bTemporarilyDisableFullDecalEffectLoading;
		if ( g_bTemporarilyDisableFullDecalEffectLoading == false )
		{
			t.decal[t.decalid].newparticle.iParticle_Floor_Active = 1;
			t.decal[t.decalid].newparticle.bParticle_Show_At_Start = false;
			t.decal[t.decalid].newparticle.bParticle_Looping_Animation = false;
			t.decal[t.decalid].newparticle.bWPE = true;
			bool preload_wicked_particle_effect(newparticletype * pParticle, int decal_id);
			if (!preload_wicked_particle_effect(&t.decal[t.decalid].newparticle, t.decalid))
			{
				//PE: Bad emitter. continue without WPE.
				... t.decal[t.decalid].newparticle.bWPE = false;
			}
		}
	}
	if (!t.decal[t.decalid].newparticle.bWPE)
	{
		// Detect and load any new particle associated with this decal
		#ifdef DETECTANDUSENEWPARTICLEDECALS
		... emittername = "gamecore\\decals\\<name>\\newparticle";  // + ".arx"
		#endif
	}
```
Note: `preload_wicked_particle_effect` returns `true` when *nothing loaded at all*
(the `if (iParticleEmitter == -1) { if (bWPE) {...} } return true;` shape at
`M-Entity_part5.cpp:20-139`), so on DX12 the version-gate failure leaves `bWPE == true`
with `emitterid == -1` — a "WPE decal that will never emit". It does **not** fall back to
`.arx`.

`t.decalelement[t.d].newparticle` inherits `bWPE` by whole-struct copy at
`M-Decal.cpp:534` (`t.decalelement[t.d].newparticle = t.decal[t.decalid].newparticle;`).

### 2.3 `bWPE` READ sites — branch table

| file:line | enclosing | `bWPE == true` (WPE / `.pe`) | `bWPE == false` (legacy `.arx` / DarkSDK decal) |
|---|---|---|---|
| `M-Entity_part5.cpp:23` | `preload_wicked_particle_effect` | build `MaxCachedDecals` clones into `ready_decals[decal_id][]`, reject non-burst emitters | nothing — function just returns `true` |
| `M-Entity_part5.cpp:159` | `newparticle_updateparticleemitter` (creation) | pull from `ready_decals[]` cache, else `WickedCall_LoadWiScene` + `Restart()` | `gpup_loadEffect(...)` + `gpup_emitterActive(...,0)` |
| `M-Entity_part5.cpp:239` | `newparticle_updateparticleemitter` (apply) | **only** `bParticle_Fire` → Translate + actions 3/4/5/1 | the full 150-line gpup override block (pos/rot/scale/offset/localrot/loop/active/speed/opacity/size/fire/bounciness/floor/colour/lifespan) |
| `M-Decal.cpp:528` | `decalelement_create` | copy profile, force `Show_At_Start`+`Fire`, set `bParticle_LocalRot_Y` = camera-facing yaw + 90°, **`t.tobj = 0`** (no decal quad object) | falls to `else if (newparticle.emitterid != -1)` gpup branch, else the legacy quad decal |
| `M-Decal.cpp:842` | `decal_update` | after 100 ms: `active = 0`, `emitterid = -1` — *"No need to stop effect it will end by itself"*; **the emitter is left in the scene** | — |
| `M-Decal.cpp:853` | `decal_update` | — | `newparticle_deleteparticleemitter(...)` → `gpup_deleteEffect` when the burst finishes |
| `M-Decal.cpp:1228` | `decal_triggerwatersplash` | one single `splashdecallargeid` element (the `.pe` does the whole splash) | the 8-element legacy composite: ripple + small/large + misty + 5× droplets + foam |
| `M-Entity_part4.cpp:1903` | `delete_notused_decal_particles` | `DeleteEmitterEffects(emitterid)` | (falls through to the `gpup_deleteEffect` loop at `:1918`) |
| `G-Entity_part2.cpp:1001` | `entity_triggerdecalatimpact`, `t.tttriggerdecalimpact == 2` (blood) | one `decalelement_create()` with `bloodsplatid` | `for (t.iter = 1; t.iter <= 3 + Rnd(1); ...) decal_triggerbloodsplat();` |
| `M-Weapon.cpp:1792` | `weapon_projectileresult_make` (explosion) | `decalelement_create()` on `t.decalglobal.newexplosion` | `explosion_custom(...)` — the hard-coded sprite/light/smoke/sparks explosion |

### 2.4 THE BOUNDARY STATEMENT

> **`newparticletype` is a *shared descriptor struct* with a one-bit discriminator, `bWPE`.
> `bWPE` is set in exactly one function — `decal_load()` in `M-Decal.cpp` — and only when
> `gamecore\decals\<name>\wpe.pe` exists on disk and successfully preloads. Nothing else in
> the codebase ever sets it. Therefore:**
>
> - **`bWPE == true` ⟺ a DECAL (bank entry or live element) that found a `wpe.pe`.**
>   This is the *entire* WPE surface reachable through `newparticletype`.
> - **`bWPE == false` ⟺ everything else**, including **every entity particle marker**
>   (`t.entityprofile[bankindex].ismarker == 10`, dispatched at `M-Entity_part5.cpp:442`)
>   — those are unconditionally the legacy `gpup_`/`.arx` path.
>
> **The single fork point is `M-Entity_part5.cpp:239`**
> (`if (pParticle->bWPE) { …WPE… } else { …gpup… }`) with its creation-side twin at `:159`.
> A replacement WPE backend needs to reimplement only the `true` arms of those two `if`s,
> plus the three functions in `wickedcalls_part4.cpp` + `WickedCall_LoadWPE`. The `else`
> arms and everything they call (`gpup_*`) can be left untouched.
>
> **Two WPE entry points bypass `newparticletype` entirely** and therefore bypass `bWPE`:
> 1. **Weapon projectiles** — `weapontype`/`WeaponProjectileBase` fields
>    `WPE_Effect` (`Types.h:1407`), `WPE_Explosion`, `WPE_MeshID`, and per-instance
>    `WPE_Root` (`Types.h:1409`, `:1562`). Read from GUNSPEC via `wpeeffect` /
>    `wpeexplosion` (`M-Weapon.cpp:970-987`). These call `WickedCall_LoadWPE` directly.
> 2. **LUA** — `WParticleEffectLoad` etc., which keep their own root list
>    `vWickedEmitterEffects` (`DarkLUA_part5.cpp:997`).
> 3. **Editor preview** — `PreviewWPERoot` (`imgui_gg_dx11_part0.cpp:70`).
>
> So the full "do not break" set for a WPE replacement is:
> `{ bWPE==true arms in M-Entity_part5.cpp, WPE_Effect/WPE_Explosion/WPE_Root in
> M-Weapon.cpp, vWickedEmitterEffects in DarkLUA_part5.cpp, PreviewWPERoot in the IMGUI
> property panel }`.

### 2.5 The decal coupling (user said "don't touch decals" — but decals own the .PE surface)

12 stock decals ship a `wpe.pe`:
```
D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Files\gamecore\decals\
  blood, dust, explosion, explosion huge, explosion large, explosion medium,
  explosion small, explosion_blood, impact, sparks, splash_large, splinters
```
Each `<name>\wpe.pe` is a *separate asset* from `Files\particlesbank\wpe\*.pe` — they are the
same format, but the decal ones are addressed by convention (`gamecore\decals\<name>\wpe.pe`,
`M-Decal.cpp:222`) rather than picked by the user.

The coupling is **one-directional and shallow**: `M-Decal.cpp` only ever
(a) tests `FileExist` on that path, (b) sets `bWPE`, (c) calls
`preload_wicked_particle_effect` / `newparticle_updateparticleemitter`, and
(d) special-cases the 100 ms teardown. It never touches `wiEmittedParticle` or any
`WickedCall_*` symbol itself. So a WPE backend swap can keep `M-Decal.cpp` byte-identical
provided the replacement preserves:
- `preload_wicked_particle_effect(newparticletype*, int decal_id)` signature + the
  `false` = "not a burst emitter, fall back" contract;
- `newparticletype::emitterid` as an opaque handle with `-1` = none, `-2` = "force reload"
  (`M-Decal.cpp:820-828`);
- the `ready_decals[decal_id][slot]` / `decal_count[decal_id]` cache convention
  (`MAXUNIQUEDECALS` × `MAXREADYDECALS`).

---

## 3. Per-instance overrides — what actually reaches a live WPE emitter

`newparticletype` (Types.h:5444-5545) exposes 40+ fields. On the **WPE branch**
(`M-Entity_part5.cpp:239-260`) the applied set is:

| Field(s) | Applied to a live WPE emitter? | Where / why not |
|---|---|---|
| `emittername` | **YES** — file path | `M-Entity_part5.cpp:182`, `:37` |
| `emitterid` | **YES** — the Wicked root Entity id | `:171`, `:209`, `:214` |
| `bParticle_Preview` | **YES (indirect)** — selects `bShowThisParticle` in editor | `:154` |
| `bParticle_Show_At_Start` | **YES (indirect)** — selects `bShowThisParticle` in test game | `:152` |
| `bParticle_Fire` | **YES** — the only actual trigger | `:245-257` → actions 3/4/5/1 |
| `iMaxCache` | **YES** — clamps the preload clone count | `:17-18`, `:162-163` |
| `bParticle_Looping_Animation` | **NO** | gpup-only (`:299-302`). WPE loop/one-shot comes from the `.pe`'s own `count`/`life`. |
| `bParticle_SpeedChange` / `fParticle_Speed` (+`_Original`) | **NO** | gpup-only (`:311-322`) |
| `bParticle_OpacityChange` / `fParticle_Opacity` (+`_Original`) | **NO** | gpup-only (`:325-336`) |
| `bParticle_SizeChange` / `bParticle_Size` (+`_Original`) | **NO** | gpup-only (`:339-350`) |
| `bParticle_ColorChange` / `fParticle_R/G/B` (+`_Original`) | **NO** | gpup-only (`:386-397`) |
| `bParticle_LifespanChange` / `fParticle_Lifespan` (+`_Original`) | **NO** | gpup-only (`:400-411`) |
| `fParticle_BouncinessChange` / `fParticle_Bounciness` (+`_Original`) | **NO** | gpup-only (`:360-371`). DX12 engine *does* have `wiEmittedParticle::restitution` (`wiEmittedParticle.h:125`) — a natural hook. |
| `iParticle_Floor_Active` / `fParticle_Floor_Height` (+`_Original`) | **NO** | gpup-only (`:372-383`). Set to 1 by `M-Decal.cpp:250` but never read on the WPE path. |
| `bParticle_Offset_Used` / `bParticle_Offset_X/Y/Z` | **NO** | gpup-only (`:265-275`) |
| `bParticle_LocalRot_Used` / `bParticle_LocalRot_X/Y/Z` | **NO — and this is a live bug** | `M-Decal.cpp:541-545` *computes and stores* a camera-facing yaw specifically for WPE decals, but the WPE arm never reads it. See §3.1. |
| `bParticle_Full_Screen`, `fParticle_Fullscreen_Duration/Fadein/Fadeout`, `Particle_Fullscreen_Transition` | **NO (neither branch)** | persisted in `.ele` and edited in the UI, but no consumer anywhere in the DX12 tree |
| position `fX/fY/fZ` | **YES** (only when `bParticle_Fire`) | `:249-251` |
| rotation `fRX/fRY/fRZ`, `pmatBaseRotation` | **NO** | gpup-only (`:295`) |
| scale `fScale` | **NO** | gpup-only (`:296`) |

**Summary: on the WPE path exactly 6 of ~30 authoring knobs do anything, and 3 of those are
just "which file / is it showing / how many clones".** Everything a user tunes in the
particle panel — speed, opacity, size, colour, lifespan, bounce, floor, offset — is
silently ignored for `.pe` content. Any replacement should treat this as the design gap to
close, not as behaviour to preserve.

### 3.1 Regression vs DX11: the camera-facing billboard was dropped

DX11 `D:\max\GameGuruMAX\GameGuru Core\GameGuru\Source\M-Entity.cpp:8813-8880` has **two**
versions of the WPE apply block: an older one commented out (`:8811-8830`, plain
`Translate`), and the live one at `:8834+` headed `//PE: Make emitter always face camera.`:
```cpp
					if (root_tranform)
					{
						root_tranform->ClearTransform();
						XMVECTOR cameraPos = XMVectorSet(CameraPositionX(0), CameraPositionY(0), CameraPositionZ(0), 0.0f);
						XMVECTOR emitterPos = XMVectorSet(fX, fY, fZ, 0.0f);
						XMVECTOR lookDir = XMVectorSubtract(cameraPos, emitterPos);
						lookDir = XMVectorSetY(lookDir, 0.0f);
						if (XMVectorGetX(XMVector3LengthSq(lookDir)) > 0.0001f)
						{
							lookDir = XMVector3Normalize(lookDir);
							XMVECTOR worldUp = XMVectorSet(0, 1, 0, 0);
							XMMATRIX lookAtMat = XMMatrixLookToLH(emitterPos, lookDir, worldUp);
							XMMATRIX worldMat = XMMatrixInverse(nullptr, lookAtMat);
							root_tranform->MatrixTransform(worldMat);
						}
						else
						{
							root_tranform->Translate(XMFLOAT3(fX, fY, fZ));
						}
						root_tranform->UpdateTransform();
					}
```
The DX12 port (`M-Entity_part5.cpp:247-252`) took the **commented-out older** variant:
```cpp
					if (root_tranform)
					{
						root_tranform->ClearTransform();
						root_tranform->Translate(XMFLOAT3(fX, fY, fZ));
						root_tranform->UpdateTransform();
					}
```
So directional `.pe` decal effects (blood spray, impact spray) will not orient toward the
camera on DX12 even once the archive-version problem is fixed. This is also why
`M-Decal.cpp:541-545`'s `bParticle_LocalRot_Y` computation is now orphaned.

---

## 4. Editor UI

There is **no dedicated WPE particle panel.** The "Particle Values" panel in
`M-GridEditB_part12.cpp:1244-1795` (marker type `ismarker == 10`) is the **gpup/.arx**
panel — its 9 built-in presets are `Predefined_Particle_Name[i] = "particlesbank\\<name>"`
(`:1284-1292`: `default`, `portal5`, `stylized_poisonring`, `smoke_billowy`,
`fountain_directional`, `fire_tornado_3`, `fire_and_smoke`, `explosion`, `smoke_thick` — all
`.arx`), and its apply block calls `gpup_loadEffect` / `gpup_setGlobalPosition` /
`gpup_setEffectAnimationSpeed` / `gpup_setEffectOpacity` directly (`:1770-1792`). **No WPE
here at all.**

WPE authoring rides entirely on the **generic LUA script-property mechanism** (DLua). A
script declares a property in its `-- DESCRIPTION:` header; the entity Properties panel
renders a widget for it.

### 4.1 The `wpefile` widget — `imgui_gg_dx11_part5.cpp:206-289`

Trigger (`:208`): `if (pestrcasestr(tmpeleprof->PropertiesVariable.Variable[i], "wpefile"))`
— any property whose **name contains** `wpefile`. `bwpefile` is set only when
`FileExist(VariableValue[i])` (`:210-214`), i.e. the preview UI hides itself if the path is bad.

| Widget | line | Does it work? |
|---|---|---|
| File picker `imgui_setpropertyfile2(1, …, "Select File", "..\\files\\")` | `:216` | **Works.** Absolute paths get rewritten relative to `szWriteDir + "Files\\"` (`:220-236`) |
| (on change) reload preview | `:241-254` | Loads, but `WickedCall_LoadWPE` returns 0 for all shipped `.pe`; actions 5 is dead |
| `Checkbox("Preview", &bPreviewWPE)` | `:259` | Load/unload works structurally; **nothing renders** (version gate) |
| `Button("Burst##Burst")` → action 1 | `:283-285` | Action 1 (`ec->Burst(0)`) is live, but `PreviewWPERoot == 0` |
| `offsety` int slider → `fPreviewYOffset` | `:888-892` | **Works** — any property named `*offsety*` on a `wpefile` entity drives the preview Y offset. Reset to 0 at `:903` when absent. |

Preview state lives in `imgui_gg_dx11_part0.cpp:69-71`:
```cpp
bool bPreviewWPE = false;
uint32_t PreviewWPERoot = 0;
float fPreviewYOffset = 0;
```
Torn down on: script change (4 sites in `M-GridEditB_part12.cpp`), selection change
(`M-GridEditB_part24.cpp:1004-1013`), and entering test game
(`M-GridEdit_part0.cpp:1641-1647`, `M-GridEdit_part1.cpp:379-385` and `:437-443`).
Per-frame reposition from `M-GridEdit_part0.cpp:1578` → `RenderPreviewEmitter()`.

### 4.2 The `effectlist` dropdown — `imgui_gg_dx11_part4.cpp:864-897`

This is the only place `Files\particlesbank\wpe` is enumerated:
```cpp
										if (stricmp(labels[1].c_str(), "effectlist") == NULL)
										{
											...
											char writePath[MAX_PATH];
											extern const char* GG_GetWritePath();
											strcpy(writePath, GG_GetWritePath());
											...
											char writableEffect[MAX_PATH];
											strcpy(writableEffect, writePath);
											strcat(writableEffect, "Files\\particlesbank\\wpe");
											CollectFilesWithExtension(".pe", writableEffect, &effectFilesWrite);

											extern char szRootDir[MAX_PATH];
											char docEffect[MAX_PATH];
											strcpy(docEffect, szRootDir);
											strcat(docEffect, "\\Files\\particlesbank\\wpe");
											CollectFilesWithExtension(".pe", docEffect, &effectFilesDoc);

											std::set<std::string> uniqueEffectFiles;
											for (const std::string& file : effectFilesWrite) {
												uniqueEffectFiles.insert(&file[strlen(writableEffect)-17]);
											}
											for (const std::string& file : effectFilesDoc) {
												uniqueEffectFiles.insert(file);
											}
											for (const std::string& file : uniqueEffectFiles) {
												labels.push_back(&file[strlen(docEffect)-17]);
											}
```
**Both trees are unioned into a `std::set`, so neither "wins" — it is a merge, deduped by
the trimmed relative name.** (The local variable names are inverted relative to their
contents: `writableEffect` holds the **Documents/write** path from `GG_GetWritePath()`,
`docEffect` holds `szRootDir`, the **EXE/build-area** path.)

`effectlist` has **zero users**: no `.lua` in `Scripts\scriptbank` and no `.fpe` declares it.
Dead UI branch today; the shipped scripts all use `WPEFILE$` instead.

### 4.3 Which tree wins at *runtime* (not in the picker)

All runtime loads funnel through `GG_GetRealPath(path, 0)`
(`wickedcalls_part3.cpp:2138`, `M-Entity_part5.cpp:38` and `:183`, `DarkLUA_part5.cpp:1081`,
`M-Decal.cpp:242`), defined at
`Dark Basic Public Shared\Dark Basic Pro SDK\Shared\File\CFileC_part0.cpp:337`.

Order of resolution:
1. Relative → absolute against `GetCurrentDirectoryA` (= the EXE root) — `:343-349`.
2. If the path starts with `szRootDir`, rebuild it under `szWriteDir` (Documents,
   `…\Documents\GameGuruApps\GameGuruMAX\`) and **redirect only if `GetFileAttributes`
   says it exists there** — `:400-417`.
3. Else try `szAddWriteDirAdditional` when `create == 0` — `:423-437`. Empty on a default
   install (`:131`).
4. Else `return 0` at `:450`, leaving the **EXE/build-area absolute path** in `fullPath`.

The `0` argument is `create` (read intent) — it suppresses the mkdir-in-Documents branch at
`:441` and enables the additional-dir fallback.

`C:\Users\leeba\Documents\GameGuruApps\GameGuruMAX\Files\particlesbank` **exists but is
empty** (one empty `user\` subdir, 0 files), so step 2 never fires and
**`.pe` files load from `D:\DEV\BUILD\GameGuru Wicked MAX Build Area\Max\Files\particlesbank\wpe\`.**
There is no skybank-style special case — skybank loads from Documents purely because its
Documents folder happens to hold 37 files (387 MB) and passes the `GetFileAttributes` test.
Same code, different data.

Standalone exception: `M-Decal.cpp:233-242` deliberately skips `GG_GetRealPath` when
`get_gameisexe() != 0` — `//PE: in standalone only check standalone folder, dont use from docwrite.`

Corollary risk: anything written into `Documents\…\Files\particlesbank\wpe\` will
**silently shadow** the build-area copy of the same filename.

---

## 5. LUA API

Registered in `addFunctions()`,
`Dark Basic Public Shared\Dark Basic Pro SDK\DarkSDKMore\DarkLUA\DarkLUA_part7.cpp:1036-1042`:
```cpp
#ifdef WICKEDPARTICLESYSTEM
	//PE: Wicked particle system.
	lua_register(lua2, "WParticleEffectLoad", WParticleEffectLoad);
	lua_register(lua2, "WParticleEffectPosition", WParticleEffectPosition);
	lua_register(lua2, "WParticleEffectVisible", WParticleEffectVisible);
	lua_register(lua2, "WParticleEffectAction", WParticleEffectAction);
#endif
```

| LUA command | signature | C fn | status |
|---|---|---|---|
| `WParticleEffectLoad(filename)` → `effectId` | 1 arg (string), 1 return | `DarkLUA_part5.cpp:1055` | **Structurally works, returns 0 in practice.** Early-out `if (g_iDisableWParticleSystem == 1) { push 0; return 1; }` (`:1062-1067`). `GG_GetRealPath(path,0)` → `WickedCall_LoadWiScene` → last emitter's `hierarchy->parentID`; `ec->Restart()`; `//ec->SetVisible(true); DX12 discontinued` (`:1102`). Pushes root onto `vWickedEmitterEffects` (`:1104`). Because the archive gate rejects every shipped `.pe`, **returns 0**. |
| `WParticleEffectPosition(id, x, y, z [, rx, ry, rz])` | 4 or 7 args, 0 returns | `DarkLUA_part5.cpp:1110` | **Works** (given a valid id). Degrees→radians, `ClearTransform`/`Translate`/optional `RotateRollPitchYaw`/`UpdateTransform` on the root. |
| `WParticleEffectVisible(id, visible)` | 2 args, 0 returns | `DarkLUA_part5.cpp:1170` | **SILENT NO-OP.** Loop body is a single commented line: `//ec->SetVisible(bVisible); DX12 discontinued` (`:1192`). |
| `WParticleEffectAction(id, action)` | 2 args, 0 returns | `DarkLUA_part5.cpp:1202` | **Partially works.** Forwards to `WickedCall_PerformEmitterAction`. Actions 1 (Burst all), 2 (Pause), 3 (Resume), 4 (Restart) live; **5/6 (visible) and 7/8 (emit pause) are no-ops.** |

Global kill switch `g_iDisableWParticleSystem` — defined `M-GridEdit_part0.cpp:331` (`= 0`),
set from `setup.ini` key `disablewparticlesystem` at `Common_part1.cpp:1147`, and *also*
parsed manually at `Common_part3.cpp:901-907` **for standalone EXEs only** (guarded by
`if(!pestrcasestr(appname,"gamegurumax.exe"))` at `:882`). Read at `DarkLUA_part5.cpp:1063`
and `M-Entity_part5.cpp:10`.

Trailing TODO list left in the source at `DarkLUA_part5.cpp:1213-1219`:
```
//disableindoor
// rotate
// Stop
// copy lua code from app.
// add emitter with all settings.
// follow mesh.
// follow bone.
```

### 5.1 Shipped scripts that depend on WPE

All five live in `D:\max\GameGuruMAXDX12\Scripts\scriptbank\particles\`:

| script | `WPEFILE$` default | commands used |
|---|---|---|
| `wpe_area.lua` | `particlesbank//wpe//firearea.pe` | `WParticleEffectLoad` :21, `WParticleEffectPosition` :61-63, `WParticleEffectAction` :68-69 |
| `wpe_blast.lua` | `particlesbank//wpe//dirt-splash4.pe` | Load :17, Position :49-50, Action :69-70 — **explosion burst** |
| `wpe_impact.lua` | `particlesbank//wpe//dirt-splash4.pe` | Load :18, Position/Visible/Action :20-21, :53-55, :70-71, :79-80 — **weapon impact** |
| `wpe_rain.lua` | `particlesbank//wpe//heavy-rain3.pe` | Load :20, :22-23, :60-64, :72-73, :81-82 — **weather** |
| `wpe_zone.lua` | `particlesbank//wpe//heavy-rain3.pe` | Load :46, Position/Visible/Action :47-49, :58-62, :77-83 |

Doc block: `Scripts\scriptbank\global.lua:2645-2648`.

`Scripts\scriptbank\markers\particle.lua` is an **empty stub** —
`-- DESCRIPTION: Just displays the particle as seen in the editor.` with empty
`particle_init`/`particle_main`; it uses no particle command at all (it relies on the
editor's `ismarker == 10` gpup emitter).

Scripts that reference `wpe_zone`/`wpe_rain` etc. rely on `WParticleEffectVisible` for
show/hide, which is the dead command — so even after fixing the archive format they will
**not** be able to hide an effect.

### 5.2 Standalone packaging awareness

`M-MapFile_part1.cpp:1320-1348` scans each entity's LUA script for
`WParticleEffectLoad(` / `WParticleEffectLoad ` and adds the quoted filename to the
collection, then resolves its sibling textures via `AddWPETextures`
(`M-MapFile_part3.cpp:736`), which appends `0_color.png` … `6_color.png` for `.pe`
(and `_g/_r/_s1/_sx.png` for `.arx`). Also `M-MapFile_part1.cpp:1262-1270` handles `.pe`
found in property variable values. `M-MapFile_part1.cpp:1183-1194`
(`mapfile_addallentityrelatedfiles`) handles `eleprof.newparticle.emittername`.
Folder-level sweeps: `M-MapFile_part1.cpp:543` `addfoldertocollection("particlesbank")`,
`M-GridEditB_part20.cpp:628`, `M-MapFile_part2.cpp:23-25`.
Note `DBDLLCore_part1.cpp:819-820` **excludes `Files\particlesbank` from encryption**
(comment: `//PE: particlesbank use GPUP_LoadTexture that do not support decrypt.`).

---

## 6. Level save / load

There is **no dedicated WPE record in the level format.** Two distinct mechanisms persist it:

### 6.1 `newparticletype` in the `.ele` binary (entity element data)

Save — `GameGuru\Source\M-Entity_part4.cpp:443-461` (function writes `t.entityelement[ent]`):
```cpp
				if (t.versionnumbersave >= 320)
				{
					writer.WriteLong( t.entityelement[ent].eleprof.newparticle.bParticle_Preview );
					writer.WriteLong( t.entityelement[ent].eleprof.newparticle.bParticle_Show_At_Start );
					writer.WriteLong( t.entityelement[ent].eleprof.newparticle.bParticle_Looping_Animation );
					writer.WriteLong( t.entityelement[ent].eleprof.newparticle.bParticle_Full_Screen );
					writer.WriteFloat( t.entityelement[ent].eleprof.newparticle.fParticle_Fullscreen_Duration );
					writer.WriteFloat( t.entityelement[ent].eleprof.newparticle.fParticle_Fullscreen_Fadein );
					writer.WriteFloat( t.entityelement[ent].eleprof.newparticle.fParticle_Fullscreen_Fadeout );
					writer.WriteString( t.entityelement[ent].eleprof.newparticle.Particle_Fullscreen_Transition.Get() );
					writer.WriteFloat( t.entityelement[ent].eleprof.newparticle.fParticle_Speed );
					writer.WriteFloat( t.entityelement[ent].eleprof.newparticle.fParticle_Opacity );

				}
				if (t.versionnumbersave >= 321)
				{
					writer.WriteString( t.entityelement[ent].eleprof.newparticle.emittername.Get() );
				}
```

Load — `GameGuru\Source\M-Entity_part3.cpp:605-622`:
```cpp
					if (t.versionnumberload >= 320)
					{
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Preview = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Show_At_Start = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Looping_Animation = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Full_Screen = t.a;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Fullscreen_Duration = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Fullscreen_Fadein = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Fullscreen_Fadeout = t.a_f;
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.newparticle.Particle_Fullscreen_Transition = t.a_s;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Speed = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Opacity = t.a_f;
					}
					if (t.versionnumberload >= 321)
					{
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.newparticle.emittername= t.a_s;
					}
```

**Format facts:**
- 11 fields, versions **320** (10 fields) and **321** (+`emittername`). Sequential, no tags —
  strictly positional, so any new field must be appended under a **new** `versionnumbersave`
  guard or every later block desyncs.
- **`bWPE` is NOT persisted.** It is recomputed at load time from `FileExist` in
  `decal_load()`. Sensible, since it is derived state.
- **`emitterid` is NOT persisted** — reset to `-1` at `M-GridEditB_part10.cpp:611`,
  `:1562`, `:1803`, `M-Entity_part2.cpp:410`.
- The following are **NOT persisted at all** (they exist only in RAM / LUA):
  `bParticle_SpeedChange`, `bParticle_OpacityChange`, `bParticle_SizeChange`/`bParticle_Size`,
  `bParticle_Offset_*`, `bParticle_LocalRot_*`, `bParticle_Fire`, `iParticle_Floor_*`,
  `fParticle_Bounciness*`, `bParticle_ColorChange`/`fParticle_R/G/B`,
  `bParticle_LifespanChange`/`fParticle_Lifespan`, all `_Original` shadows, `iMaxCache`.
- Default `emittername = "particlesbank/default"` (`Types.h:5498`, `M-Entity_part2.cpp:411`)
  — an `.arx` name, i.e. the `.ele` field is a **gpup** field that happens to be reused by
  `M-Decal.cpp` for `.pe` paths.

### 6.2 FPE text form (entity bank / group export) — `M-GridEditB_part10.cpp`

Save `:1233-1247`:
```cpp
				MakeFPELine(pLine, "objshowstart", i, cstr(t.entityelement[e].eleprof.newparticle.bParticle_Show_At_Start)); WriteString (1, pLine);
				...
				LPSTR pParticleName = t.entityelement[e].eleprof.newparticle.emittername.Get();
				...
				MakeFPELine(pLine, "objpartloop", i, cstr(t.entityelement[e].eleprof.newparticle.bParticle_Looping_Animation)); WriteString (1, pLine);
				MakeFPELine(pLine, "objpartspeed", i, cstr(t.entityelement[e].eleprof.newparticle.fParticle_Speed)); WriteString (1, pLine);
				MakeFPELine(pLine, "objpartopacity", i, cstr(t.entityelement[e].eleprof.newparticle.fParticle_Opacity)); WriteString (1, pLine);
```
Load `:1487-1492`:
```cpp
							if (ReadFPELine("objshowstart", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.newparticle.bParticle_Show_At_Start = t.value1;
							...
							if (ReadFPELine("objpartname", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.newparticle.emittername = t.value_s;
							if (ReadFPELine("objpartloop", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.newparticle.bParticle_Looping_Animation = t.value1;
							if (ReadFPELine("objpartspeed", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.newparticle.fParticle_Speed = t.value1;
							if (ReadFPELine("objpartopacity", t.field_s.Get(), &iOptionalIndex)) t.entityelement[pObjTable[iOptionalIndex].e].eleprof.newparticle.fParticle_Opacity = t.value1;
```
Keys: `objshowstart`, `objpartname`, `objpartloop`, `objpartspeed`, `objpartopacity`.

### 6.3 The actual WPE persistence: the LUA property string

Because a WPE effect is chosen via a **script property**, its filename is persisted as part
of the entity's LUA properties string — `eleprof.soundset4_s`, rebuilt as
`<scriptname>_properties(...)` at `imgui_gg_dx11_part5.cpp:905-910` and parsed back by
`InitParseLuaScript`. `M-MapFile_part1.cpp:1204` carries the note
`// PropertiesVariables may eventually be saved with entityelement data, so we could remove this step in future`,
confirming there is no first-class storage. **So: an entity's chosen `.pe` file survives a
save/load only as text inside its script-properties call.**

### 6.4 Decal WPE persistence

None. Decal `.pe` binding is pure convention: `gamecore\decals\<name>\wpe.pe` discovered by
`FileExist` in `decal_load()` every session (`M-Decal.cpp:222-266`). Nothing about it is
written to the level.

### 6.5 Weapon WPE persistence

GUNSPEC text fields, read at `M-Weapon.cpp:970-987`: `wpeeffect` → `WPE_Effect`,
`wpeexplosion` → `WPE_Explosion`. Not level data.

---

## 7. Runtime spawn sites

| Trigger | Path | WPE? | Working? |
|---|---|---|---|
| **Weapon impact (material)** | `G-Entity_part2.cpp:986-995` → `decal_triggermaterialdecal()` / `decal_triggerimpact()` → `decalelement_create()` → `M-Decal.cpp:528` `bWPE` | via `gamecore\decals\impact\wpe.pe` | Archive gate → falls through to a `bWPE == true` decal with `emitterid == -1`; **nothing spawns, and no `.arx` fallback either** |
| **Blood** | `G-Entity_part2.cpp:1001` | `gamecore\decals\blood\wpe.pe` | same; the `else` at `:1008` would do 3-4 legacy `decal_triggerbloodsplat()` but is not taken because `bWPE` is still true |
| **Explosion** | `M-Weapon.cpp:1792` `else if (t.decalglobal.newexplosion > 0 && …bWPE)` → `decalelement_create()` | `gamecore\decals\explosion*\wpe.pe` | same; falls past `explosion_custom()` at `:1805` |
| **Water splash** | `M-Decal.cpp:1228` `decal_triggerwatersplash()` | `gamecore\decals\splash_large\wpe.pe` | same; the rich 8-element legacy composite at `:1234-1247` is skipped |
| **Projectile trail** | `M-Weapon.cpp:1215` `WickedCall_LoadWPE(WPE_Effect)`; positioned `:391/:411/:1367`; fired `:1370-1372`; stopped `:1693` | direct, no `bWPE` | `WPE_Root == 0` (archive gate) so all guards `if (WPE_Root > 0)` fail. Even with a valid root, the stop (`action 7`) is dead. |
| **Weather / rain / area effects** | `Scripts\scriptbank\particles\wpe_rain.lua`, `wpe_zone.lua`, `wpe_area.lua` → `WParticleEffectLoad` | LUA | returns 0. Also `WickedCall_UpdateEmitters`'s follow-camera/find-floor logic is `if(false)`. |
| **Footsteps** | — | no WPE path exists | n/a |
| **Editor preview** | `imgui_gg_dx11_part5.cpp:265` | direct | loads nothing |
| **Fullscreen particle overlay** | `bParticle_Full_Screen` + fade/transition fields | neither | **no consumer at all** in the DX12 tree — persisted and editable, entirely inert |

**Critical behavioural note:** because `preload_wicked_particle_effect` returns `true` when
the load produced no emitter (`M-Entity_part5.cpp:20-139` — the `for` loop simply never sets
`root`), `decal_load()` leaves `bWPE == true`. The `if (!bWPE)` fallback to `.arx`
(`M-Decal.cpp:266`) and every `else` legacy-decal branch listed above is therefore **skipped**.
The 12 stock decals that ship a `wpe.pe` currently render **no particle effect of either
kind**. Making `preload_wicked_particle_effect` return `false` when `root` is never assigned
would restore the legacy fallback as a one-line safety net.

---

## 8. Files to touch for a replacement (ranked)

1. `Guru-WickedMAX\wickedcalls_part4.cpp` — the whole WPE call layer (407 lines; 5 real
   functions + 2 dead ones).
2. `Guru-WickedMAX\wickedcalls_part3.cpp:1950-2162` — `WickedCall_LoadWiSceneDirect` /
   `LoadWiScene` / `LoadWPE`, incl. the archive-version gate.
3. `GameGuru\Source\M-Entity_part5.cpp:1-140` (`preload_wicked_particle_effect`) and
   `:239-260` (the WPE apply arm). **The single fork point.**
4. `DarkLUA\DarkLUA_part5.cpp:996-1220` — `DeleteEmitterEffects`, `CleanUpEmitterEffects`,
   the 4 `WParticleEffect*` commands.
5. `GameGuru\Imgui\imgui_gg_dx11_part5.cpp:206-289` + `:886-903` — the `wpefile` widget.
6. `GameGuru\Source\M-Weapon.cpp` — `WPE_Effect`/`WPE_Root` at `:389-411`, `:1212-1218`,
   `:1363-1373`, `:1686-1695`.
7. `GameGuru\Source\M-GridEditB_part24.cpp:995-1026` — `RenderPreviewEmitter`.
8. `GameGuru\Imgui\imgui_gg_dx11_part4.cpp:864-897` — the (currently unused) `effectlist`
   browser.

**Do NOT touch:** `Guru-WickedMAX\GPUParticles*` (`gpup_`/`.arx`),
`GameGuru\Source\M-Particles.cpp` (`ravey_particles_*`/`CParticleC`), the `else` arms in
`M-Entity_part5.cpp`, and `M-Decal.cpp` — the last of these only needs the
`preload_wicked_particle_effect` contract honoured.
