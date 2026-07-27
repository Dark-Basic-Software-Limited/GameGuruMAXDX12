# AMD Radeon GPU Detective (RGD) — post-crash capture setup

Purpose: when the GPU driver TDRs (e.g. the 2026-07-27 23:07 `DXGI_ERROR_DEVICE_HUNG`
during the Canyon Adventure terrain build), RGD produces a driver-level dump naming the
exact in-flight draw/dispatch, the page-fault address, and the full create/bind/destroy
history of whichever resource owned that memory. Pairs with the engine-side DRED capture
(Wicked delta 1.51 — `dred.txt` next to the EXE → `dred_report.txt` on device removal).

## Support facts (verified 2026-07-27)

- RX 9060 XT supported since **RGD v1.5** (2025-06-17); current release **v1.6.3** (2026-06-11).
- Requires **Adrenalin 25.10.2+** for the current suite. **Reboot after any driver install**
  before capturing — capturing right after an install produces invalid dumps (known issue).
- **Caution**: AMD currently lists **Windows 11** as the supported OS for the tool suite; this
  dev rig is Windows 10 19045. Nothing documents a hard block (capture rides on the driver),
  so try it — if the suite misbehaves on Win10, the engine DRED capture is the fallback.
- The crash must actually TDR (ours did — Windows Event Log `Display` 4101) and the Windows
  `TdrLevel` registry value must be the default (3 = recover). DX12 apps only (MAX qualifies).

## Setup (one-time)

1. Update AMD Software Adrenalin to the latest (>= 25.10.2), then REBOOT.
2. Download the **Radeon Developer Tool Suite** zip (contains RDP + rgd.exe):
   https://gpuopen.com/radeon-gpu-detective/
3. Run `RadeonDeveloperPanel.exe` from the zip:
   - CAPTURE → Available features → enable **Crash Analysis**.
   - Crash Analysis tab → check **"Text"** (auto-writes the readable summary next to the dump)
     and set the dump output folder.
   - Leave **Hardware crash analysis** checked (adds shader-level SHADER INFO when the crash
     originates in a shader).
   - In the **Applications** panel on the same CAPTURE tab, set Auto connect + API (e.g.
     "Existing applications" + "DirectX 12") and make sure GameGuruMAX.exe is in the list.
     (RDP v3.5 note: older docs say SYSTEM → "Global workflow" — v3.5 moved this; the SYSTEM
     tab now only holds Device Clocks / Driver Experiments. The CAPTURE Applications panel is
     the config point.)

**VERIFIED WORKING 2026-07-27 ~23:57 on the dev rig (RDP v3.5.0.18, Windows 10)**: with RDP
open, launching MAX turns the Crash Analysis page to green **"Status: Capturing"** with the
GameGuruMAX.exe PID attached. Note: device creation takes ~3.1s in this mode (vs ~0.1s
normal) — that's the crash-analysis instrumentation loading at startup; runtime cost is small.

## Capture (each repro attempt)

4. Start RDP **first**, then launch GameGuruMAX.exe normally — the driver auto-detects any
   DX12 app while RDP runs (shows as "Active" in RDP). RDP must stay running.
5. Reproduce the hang (the suspect path: activity on level 1 → in-place load of level 2).
6. After the TDR, the `.rgd` dump + text summary appear in the configured folder / RDP's
   "Recently collected dumps".

## Reading the dump

- Auto text summary is usually enough: execution-marker tree per in-flight command buffer
  (finished / in-progress / not-started, `[#]` = shader actively executing), page-fault VA +
  the resources that owned it over time.
- Manual conversion / extras:
  - `rgd --parse crash.rgd -o report.txt`
  - flags: `--expand-markers`, `--marker-src`, `--va-timeline`, `--all-resources`
- Wicked already names its D3D12 resources and emits PIX markers (`device->EventBegin`),
  which RGD v1.6.3 consumes natively — expect a readable marker tree with zero code changes.

## Engine-side DRED (always-on fallback, any machine)

- Arm: empty `dred.txt` next to GameGuruMAX.exe (currently ARMED on the dev rig).
- On any device removal: report appends to `dred_report.txt` next to the EXE (removal reason,
  last in-flight op per command list, page-fault VA + owning live/freed resources).
- Disarm: delete `dred.txt` (breadcrumb writes cost a few % GPU while armed).
