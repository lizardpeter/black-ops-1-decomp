# Mouse input

How mouse movement becomes view angles in BO1, from the reconstruction of `win32/win_input.cpp` and the mouse
functions of `client_mp/cl_input_mp.cpp`, `client_mp/cl_cgame_mp.cpp` and `aim_assist/aim_assist.cpp`
(reference build CoDMPServer, all differential-tested; `records/functions.tsv`). The retail exe (t5mp) uses
the same scheme: its matched `IN_MouseMove` (008BAE50) and `IN_RecenterMouse` (005D3E00) do the same calls.

## Reading the mouse (win32/win_input.cpp)

Cursor based, no raw input. Every frame `IN_Frame` calls `IN_MouseMove`, which, while the game window is in
the foreground:

1. reads the cursor position (`GetCursorPos`) and, in fullscreen, clamps it to the window rectangle
   (`IN_ClampMouseMove`, moving the cursor back with `SetCursorPos` when it was outside);
2. takes the difference from the last position as this frame's movement, in screen pixels;
3. passes position and movement to `CL_MouseEvent`;
4. if the client used the movement (not in a menu), puts the cursor back in the window centre
   (`IN_RecenterMouse`) and remembers the centre as the last position.

So Windows applies its pointer speed and "Enhance pointer precision" acceleration before the game sees
anything, movement is whole pixels, and one frame's movement can't exceed the distance to the screen edge (or
the window edge in fullscreen). Buttons arrive separately as `IN_MouseEvent(mstate)`, which queues key events
200..204 (`Sys_QueEvent`) for the bits that changed.

This dedicated-server build compiles some of it out: `IN_InitWin32Mouse` and `CL_GamepadIgnoreMouse` are empty
(identical-code-folded with 293 other empty functions) and `IN_ActivateMouse` doesn't capture or hide the
cursor. Retail keeps those calls (t5mp's `IN_Frame` at 00521D30 calls two activate/deactivate helpers the
server lacks).

## From movement to angles (client_mp)

`CL_MouseEvent(x, y, dx, dy)`: the developer GUI takes the movement if it is open; a menu with UI key focus
takes the position (`UI_MouseEvent`) and the cursor is not recentred; otherwise the system cursor is hidden and
`dx, dy` are added to `cl->mouseDx/Dy[cl->mouseIndex]`.

Once per frame, `CL_MouseMove(localClientNum, cmd)`:

```
mx, my  = CL_GetMouseMovement(cl)      // this frame's counts; with m_filter the average of this and the
                                       // previous frame. The other slot is cleared and becomes current.
if frame_msec == 0: return             // movement already consumed: dropped (B0001)
rate    = sqrt(mx*mx + my*my) / frame_msec                       // counts per millisecond
sens    = (rate * cl_mouseAccel + cl_sensitivity) * cgameFOVSensitivityScale
if pm_flags & 0x800 or weaponstate == 35: return                 // view locked
mx *= sens;  my *= sens
yaw    -= clamp(m_yaw * mx,   ±frame_msec * cgameMaxYawSpeed   / 1000)   // cap only when cgame sets one
pitch  += clamp(m_pitch * my, ±frame_msec * cgameMaxPitchSpeed / 1000)
```

- `+strafe` held: horizontal movement becomes `cmd->rightmove += round(m_side * mx)` instead of yaw.
- Vertical movement is pitch when mouse look is on (`+mlook` or `cl_freelook`), otherwise
  `cmd->forwardmove -= round(m_forward * my)`. Those roundings are x87 round-to-nearest with a tiny upward
  bias (the reconstruction's `CL_MouseMoveToInt`).
- `cmd->pitchmove`/`yawmove` get the scaled movement truncated, capped at 127 before the add, then clamped to a
  signed char (`ClampChar`).
- `cgameFOVSensitivityScale` is set by cgame through `CL_SetFOVSensitivityScale`: `tan(fov/2) / tan(cg_fov/2)`,
  lower while aiming down sights (docs/fov.md).
- Unless `eFlags2 & 0x40000`, mouse aim assist runs last (`AimAssist_UpdateMouseInput`): for the mouse it only
  does auto-melee; the angles pass through unchanged.

`frame_msec` is the frame time in whole milliseconds, so acceleration and the speed caps are quantized to
milliseconds (B0003).

## Findings

All in `records/bugs.tsv`, for the enhanced build:

| id | priority | what |
|---|---|---|
| B0001 | high | movement dropped on frames under 1 ms |
| B0002 | high | cursor-based input: Windows pointer speed and acceleration apply, pixel resolution, edge and window clamping |
| B0003 | medium | acceleration and view speed caps use whole milliseconds |
| B0004 | low | uninitialized melee-charge values copied into the usercmd when aim assist isn't initialized |
| B0005 | medium | two layouts of `WinVars_t` in one build (ODR violation) |
| B0006 | low | `ShowCursor` called on every mouse event |
| B0007 | low | `m_filter` smoothing depends on frame rate |
| B0008 | low (keep) | aim-assist input always uses local client 0 |

## Not reconstructed yet

`CL_CreateCmd` (where `frame_msec` is computed and `CL_MouseMove` is called), the window procedure that
delivers `IN_MouseEvent`, the rest of `cl_input_mp.cpp`, and the retail differences above.
