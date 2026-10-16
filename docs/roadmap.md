# Roadmap: enhanced build

Wanted changes for the enhanced build (faithful code plus documented overrides; AGENTS.md "Faithful and
enhanced"). None is implemented yet: `enhanced/` and `records/patches.tsv` are created with the first one.
"Needs" lists what has to be decompiled and verified first, because changes go on verified faithful code.

| # | Change | Kind | Fixes | Needs |
|---|---|---|---|---|
| 1 | Raw mouse input: Raw Input (`WM_INPUT`/`GetRawInputBuffer`) counts instead of cursor positions; no Windows pointer speed or acceleration, no pixel rounding, no edge or window clamping, no recentring | modernize | B0002, B0006 | win32/win_input.cpp (done), the window procedure, `CL_CreateCmd` |
| 2 | Keep mouse movement on sub-millisecond frames: accumulate until a frame with time, or use a time base finer than milliseconds | bugfix | B0001 | `CL_CreateCmd`, `CL_MouseMove` (done) |
| 3 | Time-based acceleration, speed caps and filtering (seconds as float, not whole milliseconds and frames) | modernize | B0003, B0007 | `CL_CreateCmd`, `CL_MouseMove` (done), `CL_GetMouseMovement` (done) |
| 4 | Initialize the aim-assist output before use | bugfix | B0004 | `CL_MouseMove` (done) |
| 5 | One `WinVars_t` layout everywhere | bugfix | B0005 | the translation units using the 32-byte layout (to be identified) |
| 6 | Any aspect ratio: `aspectRatioWindow` from the real window or monitor size instead of three presets, so ultrawide and 5:4/3:2 get square pixels and the Hor+ FOV continues past 16:9 | modernize | B0010 | gfx_d3d/r_init.cpp `R_StoreWindowSettings` (done), the UI's 16:9 assumptions (to be checked) |
| 7 | Wider FOV setting (e.g. 65..120), and optionally a horizontal-FOV mode that states FOV for the actual screen | modernize | B0011 | `CG_RegisterDvars`, cgame/cg_camera.cpp (done), the view model's FOV compensation (cg_weapons.cpp) |
| 8 | Deploy and turret FOV relative to the player's FOV; ADS zoom blended in tan(fov/2) | bugfix | B0012 | cgame/cg_camera.cpp `CG_GetViewFov` (done) |
| 9 | Reversed-Z depth (float depth buffer, near and far swapped in the projection) | modernize | B0013 | gfx_d3d/r_scene.cpp (done for the projection), depth-buffer setup and every depth test/compare in the renderer |
