"""Differential cases for win32/win_input.cpp.

Every input carries a state: the mouse globals (s_wmv, window centre, app-active flag), the dvars the code
reads (in_mouse, r_fullscreen, gpad_present) and the harness's window model (window rectangle, cursor,
foreground window, cursor-show counter). The same state is set up before the original and the
reconstruction run; their Win32 calls, global writes and final window state are compared."""
import random
import struct

from fixtures import make_dvar, static, sym

R = random.Random(0x1A9)  # fixed seed: runs are reproducible
N = 120

SRC = "win32/win_input.cpp"
S_WMV = static(SRC, "", "s_wmv")
CENTER_X = static(SRC, "", "window_center_x")
CENTER_Y = static(SRC, "", "window_center_y")
APPACTIVE = static(SRC, "", "in_appactive")
G_WV = sym("?g_wv@@3UWinVars_t@@A")
G_SHOWCURSOR = sym("?g_showCursor@@3HA")
IN_MOUSE = sym("?in_mouse@@3PBUdvar_s@@B")
R_FULLSCREEN = sym("?r_fullscreen@@3PBUdvar_s@@B")
GPAD_PRESENT = sym("?gpad_present@@3PBUdvar_s@@B")
SV_MAPNAME = sym("?sv_mapname@@3PBUdvar_s@@B")
# win32/win_gamepad.cpp dvars, read by GPad_UpdateAll and friends (original code in both runs): kind per dvar
GPAD_DVARS = {
    "gpad_debug": "int", "gpad_enabled": "bool", "gpad_rumble": "bool", "gpad_buttonsConfig": "int",
    "gpad_sticksConfig": "int", "gpad_stick_pressed": "float", "gpad_stick_pressed_hysteresis": "float",
    "gpad_stick_deadzone_min": "float", "gpad_stick_deadzone_max": "float", "gpad_button_deadzone": "float",
    "gpad_button_lstick_deflect_max": "float", "gpad_button_rstick_deflect_max": "float",
    "gpad_menu_scroll_delay_first": "int", "gpad_menu_scroll_delay_rest": "int",
}


def coord():
    return R.choice([R.randint(-3000, 3000), R.randint(-5, 5), R.randint(0, 2000)])


def random_rect():
    left, top = coord(), coord()
    return (left, top, left + R.choice([1, 2, 3, R.randint(1, 4000)]), top + R.choice([1, 2, 3, R.randint(1, 3000)]))


def random_state():
    rect = random_rect()
    near = lambda lo, hi: R.randint(lo - 50, hi + 50)  # noqa: E731
    return {
        "init": R.random() < 0.85, "active": R.random() < 0.5,
        "old_buttons": R.choice([0, R.getrandbits(5), R.getrandbits(32)]),
        "old_pos": (coord(), coord()), "center": (coord(), coord()),
        "appactive": R.choice([0, 1, R.randint(-5, 5)]),
        "rect": rect, "cursor": (near(rect[0], rect[2]), near(rect[1], rect[3])),
        "foreground": R.random() < 0.8, "show_count": R.randint(-4, 4),
        "in_mouse": R.random() < 0.85, "fullscreen": R.random() < 0.5,
        "gpad_present": R.choice([None, False, True]),
        "msg_time": R.getrandbits(32), "recenter": R.choice([0, 1]), "g_showcursor": R.randint(-1, 2),
        "pad": random_pad(), "gpad": random_gpad_dvars(), "time_ms": R.getrandbits(31),
        "mapname": R.choice(["mp_nuketown", "mp_array", "menu_main", "MENU_x", "ui", ""]),
    }


def stick():
    return R.choice([0, 32767, -32768, R.randint(-32768, 32767), R.randint(-9000, 9000)])


def random_pad():
    if R.random() < 0.25:
        return None
    return {"packet": R.getrandbits(32), "buttons": R.choice([0, R.getrandbits(16)]),
            "lt": R.choice([0, 255, R.getrandbits(8)]), "rt": R.choice([0, 255, R.getrandbits(8)]),
            "lx": stick(), "ly": stick(), "rx": stick(), "ry": stick()}


def random_gpad_dvars():
    out = {}
    for name, kind in GPAD_DVARS.items():
        if kind == "bool":
            out[name] = R.random() < 0.8
        elif kind == "int":
            out[name] = R.choice([0, 0, 1, R.randint(0, 3)]) if name.startswith(("gpad_debug", "gpad_")) else 0
        else:
            out[name] = R.choice([0.0, 0.1, 0.2, 0.25, 0.9, R.random()])
    out["gpad_debug"] = 0  # debug output goes to Com_Printf
    return out


def setup(fx, st):
    fx.u32(S_WMV + 0x00, st["old_buttons"])
    fx.i32(S_WMV + 0x04, st["old_pos"][0])
    fx.i32(S_WMV + 0x08, st["old_pos"][1])
    fx.u8(S_WMV + 0x0C, st["active"])
    fx.u8(S_WMV + 0x0D, st["init"])
    fx.i32(CENTER_X, st["center"][0])
    fx.i32(CENTER_Y, st["center"][1])
    fx.i32(APPACTIVE, st["appactive"])
    fx.i32(G_SHOWCURSOR, st["g_showcursor"])
    fx.u32(G_WV + 0x08, fx.win["hwnd"])        # g_wv.hWnd
    fx.i32(G_WV + 0x18, st["recenter"])        # g_wv.recenterMouse
    fx.u32(G_WV + 0x20, st["msg_time"])        # g_wv.sysMsgTime (36-byte WinVars_t, see win_local.h)
    fx.win["rect"] = st["rect"]
    fx.win["cursor"] = st["cursor"]
    fx.win["foreground"] = fx.win["hwnd"] if st["foreground"] else 0x00020077
    fx.win["show_count"] = st["show_count"]
    make_dvar(fx, IN_MOUSE, enabled=st["in_mouse"])
    make_dvar(fx, R_FULLSCREEN, enabled=st["fullscreen"])
    if st["gpad_present"] is None:
        fx.u32(GPAD_PRESENT, 0)
    else:
        make_dvar(fx, GPAD_PRESENT, enabled=st["gpad_present"])
    for name, v in st["gpad"].items():
        kind = GPAD_DVARS[name]
        g = sym(f"?{name}@@3PBUdvar_s@@B")
        if kind == "bool":
            make_dvar(fx, g, enabled=v)
        elif kind == "int":
            make_dvar(fx, g, integer=v)
        else:
            make_dvar(fx, g, value=v)
    make_dvar(fx, SV_MAPNAME, string=st["mapname"])  # Com_IsMenuLevel (gamepad button handling)
    fx.win["pad"] = st["pad"]
    fx.win["time_ms"] = st["time_ms"]


def inputs(make_args=lambda: []):
    return [{"args": make_args(), "state": random_state()} for _ in range(N)]


def case(symbol, make_args=lambda: [], ret="void"):
    return {"symbol": symbol, "ret": ret, "setup": setup, "inputs": inputs(make_args)}


def point():
    return struct.pack("<2i", coord(), coord())


CASES = [
    case("?TRACK_win_input@@YAXXZ"),
    case("?IN_StartupGamepads@@YAXXZ"),
    case("?IN_GamepadsMove@@YAXXZ"),
    case("?IN_RecenterMouse@@YAXXZ"),
    case("?IN_DeactivateWin32Mouse@@YAXXZ"),
    case("?IN_IsForegroundWindow@@YAHXZ", ret="int"),
    case("?IN_ActivateMouse@@YAXH@Z", lambda: [R.choice([0, 1, R.randint(-9, 9)])]),
    case("?IN_DeactivateMouse@@YAXXZ"),
    case("?IN_StartupMouse@@YAXXZ"),
    case("?IN_MouseEvent@@YAXH@Z", lambda: [R.choice([R.getrandbits(5), R.getrandbits(32), 0])]),
    case("?IN_ClampMouseMove@@YAXPAUtagPOINT@@@Z", lambda: [point()]),
    case("?IN_MouseMove@@YAXXZ"),
    case("?IN_SetCursorPos@@YAXHH@Z", lambda: [coord(), coord()]),
    case("?IN_ShowSystemCursor@@YAXH@Z", lambda: [R.choice([0, 1, R.randint(-9, 9)])]),
    case("?IN_Startup@@YAXXZ"),
    case("?IN_Shutdown@@YAXXZ"),
    case("?IN_Init@@YAXXZ"),
    case("?IN_Activate@@YAXH@Z", lambda: [R.choice([0, 1, R.randint(-9, 9)])]),
    case("?IN_Frame@@YAXXZ"),
    case("?IN_InitWin32Mouse@@YAXXZ"),
]
