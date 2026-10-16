# Field of view

How BO1 turns its FOV setting into a projection, from the reconstruction of `cgame/cg_camera.cpp` and
`cgame/cg_main.cpp` (FOV), `gfx_d3d/r_scene.cpp` (view setup) and `gfx_d3d/r_init.cpp` (aspect ratio), all
differential-tested in build CoDMPServer (`records/functions.tsv`). The retail exe (t5mp) has the same
`CG_CalcFov` (matched at 00421960, with the camera tween inlined), `R_SetViewParmsForScene` (006E81F0) and
`R_StoreWindowSettings` (006D8240).

## The chain

```
cg_fov_default (65..80, the menu setting)  --CG_GetDefaultFovForView / CG_UpdateFov-->  cg_fov
CG_GetViewFov      cg_fov, narrowed for ADS zoom, widened toward 55 while deploying, 55 on turrets,
                   vehicle camera FOV; x cg_fovScale; at least cg_fovMin                (degrees)
CG_CalcFov         fov -> refdef.tanHalfFovX/Y, refdef.fov_x, cg.zoomSensitivity
CG_DrawActive      CL_SetFOVSensitivityScale(zoomSensitivity [x shellshock sensitivity])  -> mouse (docs/input.md)
R_SetViewParmsForScene   view matrix, near plane, R_SetupProjection -> InfinitePerspectiveMatrix
```

## Degrees are 4:3 horizontal ("Hor+")

`cg_fov` is the horizontal FOV of a 4:3 view. `CG_CalcFov`:

```
t           = tan(fov * pi/360)                 // float pi/180 = 0.017453292f, the CRT's SSE2 tan
tanHalfFovY = t * 0.75                          // vertical extent fixed by the 4:3 width
tanHalfFovX = tanHalfFovY * cgs.viewAspect      // horizontal extent follows the screen
zoomSensitivity = t / tan(cg_fov * pi/360)
```

So the vertical FOV is constant and wider screens see more at the sides. Horizontal FOV you actually get:

| cg_fov | 4:3 | 16:10 | 16:9 | 21:9 (if it were supported) |
|---|---|---|---|---|
| 65 (default) | 65.0 | 74.8 | 80.7 | 96.2 |
| 80 (menu maximum) | 80.0 | 90.4 | 96.4 | 111.5 |

`viewAspect` is the renderer's `vidConfig.aspectRatioWindow`, and that is only ever 4:3, 16:10 or 16:9
(`R_StoreWindowSettings`): on auto, `round(16 * height / width)` of the monitor's native mode (fullscreen) or
the window picks 16:10 when it is 10, 16:9 below and 4:3 above. 21:9 and 32:9 get 16:9 and 5:4 and 3:2 get
4:3, with the pixel aspect (`aspectRatioScenePixel`) absorbing the difference, so the image is stretched or
squashed (B0010).

## CG_GetViewFov

In order, for the local player's predicted state:

1. `viewFov = cg_fov` (asserted 1..160). Demo third-person and movie cameras, script camera modes and the
   third-person model view return it unchanged (`CG_ShouldDefaultViewFov`).
2. Aim-down-sights weapons: the zoom FOV is one of `fAdsZoomFov1..3`, chosen by `adsZoomSelect` (signed `% 4`
   with `cg_adsZoomToggleStyle` 0, `% 3` with 1), never wider than `viewFov`. At `fWeaponPosFrac == 1` the FOV
   is the zoom FOV; during the raise it blends linearly in degrees over the last `fAdsZoomInFrac`
   (`fAdsZoomOutFrac` when lowering) of the weapon's travel.
3. Deploying (weaponstate 35, first person, a real weapon): `cg_fov + (55 - cg_fov) * (1 - weaponTime /
   deployTime)`, replacing step 2's result.
4. On a turret (`eFlags & 0x300`): 55. In a vehicle seat with its own camera (`eFlags & 0x4000`): the vehicle's
   `cameraFOV` if it is above `cg_fovMin`.
5. `* cg_fovScale`, then at least `cg_fovMin`.

The fixed 55s ignore the player's FOV (B0012).

## Mouse sensitivity while zoomed

`zoomSensitivity = tan(fov/2) / tan(cg_fov/2)`, both in 4:3 terms, multiplies the mouse sensitivity
(`cgameFOVSensitivityScale` in `CL_MouseMove`). That keeps the same mouse movement turning the view by the
same fraction of the screen near the crosshair (a "0% monitor distance" match), independent of the aspect
ratio. Kill cams pass their own FOV and get the matching scale.

## Projection

`R_SetViewParmsForScene`: origin and axes from the refdef, `MatrixForViewer` for the view matrix, near plane =
the refdef's if positive, else `r_znear`, and never below 1 (`MINIMUM_Z_NEAR`). `InfinitePerspectiveMatrix`:

```
m00 = k / tanHalfFovX   m11 = k / tanHalfFovY   m22 = k   m23 = 1   m32 = -zNear * k     k = 1 - 2^-11
```

a left-handed D3D projection with the far plane at infinity and ordinary depth (B0013). The view model uses
its own near plane (`depthHackNearClip = -r_znear_depthhack`). For tiled hi-res screenshots
`R_SetupViewProjectionMatrices` scales and offsets the projection to the current tile.

## Findings

All in `records/bugs.tsv`:

| id | priority | what |
|---|---|---|
| B0010 | high | only 4:3, 16:10 and 16:9: ultrawide is stretched 16:9, 5:4 and 3:2 squashed 4:3 |
| B0011 | medium | FOV setting limited to 65..80 (4:3), at most 96 degrees horizontal at 16:9 |
| B0012 | low | deploy and turret FOV fixed at 55 whatever the player's FOV; ADS zoom blends in degrees |
| B0013 | low | infinite far plane with ordinary depth: z-fighting at distance |
| B0014 | low (keep) | unused `Sys_Milliseconds` call every ADS frame |

## Not reconstructed yet

`CG_DrawActive` (where the zoom sensitivity reaches the client), `CG_CalcFov_ExtraCam`, the kill-cam callers in
`cgame_mp/cg_view_mp.cpp`, the weapon view model's FOV compensation (`cg_fovCompMax`, cg_weapons.cpp), and the
rest of the renderer.
