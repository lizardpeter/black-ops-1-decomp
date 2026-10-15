"""Differential cases for the client mouse path: client_mp/cl_input_mp.cpp (CL_MouseEvent, CL_SetCursorPos,
CL_GetMouseMovement, CL_MouseMove), client_mp/cl_cgame_mp.cpp (CL_SetFOVSensitivityScale) and
aim_assist/aim_assist.cpp (AimAssist_UpdateMouseInput).

Every input carries a state: local client 0's clientActive_t (built in fixture memory; `clients` points at it),
the UI key catchers, the mouse dvars, frame_msec, the strafe/mlook buttons, and aim assist's globals."""
import random
import struct

from fixtures import make_dvar, static, sym
from harness import FIXTURE

R = random.Random(0x3A0C5E)  # fixed seed: runs are reproducible
N = 300

CLIENTS = sym("?clients@@3PAUclientActive_t@@A")              # clientActive_t *clients (allocated at run time)
CLIENT_UI = sym("?clientUIActives@@3PAUclientUIActive_t@@A")  # clientUIActive_t clientUIActives[1]
FRAME_MSEC = sym("?frame_msec@@3IA")
CLS = sym("?cls@@3UclientStatic_t@@A")
G_WV = sym("?g_wv@@3UWinVars_t@@A")
PLAYERS_KB = static("client_mp/cl_input_mp.cpp", "", "playersKb")
AA_GLOB = static("aim_assist/aim_assist.cpp", "", "aaGlobArray")
WEAPON_VARIANT_DEFS = sym("?bg_weaponVariantDefs@@3PAPAUWeaponVariantDef@@A")  # WeaponVariantDef *[], from fastfiles
WEAPON_DEF_SIZE, WEAPON_VARIANT_DEF_SIZE = 2056, 228  # PDB
DVARS = {name: sym(f"?{name}@@3PBUdvar_s@@B") for name in (
    "m_filter", "cl_mouseAccel", "cl_sensitivity", "cl_showMouseRate", "m_yaw", "m_pitch", "m_side", "m_forward",
    "cl_freelook")}

CL_SIZE = 0x3000   # covers every clientActive_t member the mouse path touches (up to viewangles at 0x2808)
CL_VA = FIXTURE    # the client is the first fixture allocation, so its address is known when building args
KB_STRAFE, KB_MLOOK, KBUTTON_SIZE = 8, 13, 0x18


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def angle():
    return f32(R.choice([R.uniform(-180, 180), R.uniform(-1e5, 1e5), 0.0, -0.0, 89.9, -89.9, 360.0]))


def counts():
    return R.choice([0, 0, 1, -1, R.randint(-5, 5), R.randint(-200, 200), R.randint(-5000, 5000)])


def random_state(aim_assist="auto"):
    st = {
        "mouse_dx": [counts(), counts()], "mouse_dy": [counts(), counts()], "mouse_index": R.choice([0, 1]),
        "fov_scale": f32(R.choice([1.0, 1.0, 0.75, 0.5, 0.33, 0.0, R.uniform(0, 2)])),
        "max_pitch": f32(R.choice([0.0, 0.0, 0.0, 90.0, 360.0, 1000.0, R.uniform(0, 500), -10.0])),
        "max_yaw": f32(R.choice([0.0, 0.0, 0.0, 90.0, 360.0, 1000.0, R.uniform(0, 500), -10.0])),
        "viewangles": [angle(), angle(), angle()],
        "pm_flags": R.choice([0, 0, 0, 0x800, R.getrandbits(32) & ~0x800]),
        "weaponstate": R.choice([0, 0, 0, 35, R.randint(0, 40)]),
        "aim_skip": R.random() < 0.3,
        "key_catchers": 0,
        "frame_msec": R.choice([0, 1, 1, 2, 3, 4, 7, 8, 16, 33, 50, R.randint(0, 250)]),
        "frametime": R.choice([0, 1, 4, 16, 33, R.randint(0, 250)]),
        "strafe": R.random() < 0.15, "mlook": R.random() < 0.3,
        "dvars": {
            "m_filter": R.random() < 0.3, "cl_showMouseRate": False, "cl_freelook": R.random() < 0.85,
            "cl_mouseAccel": f32(R.choice([0.0, 0.0, 0.0, 0.1, 0.5, R.uniform(0, 1)])),
            "cl_sensitivity": f32(R.choice([5.0, 3.0, 10.0, 1.0, 0.5, R.uniform(0, 30)])),
            "m_yaw": f32(R.choice([0.022, 0.022, -0.022, R.uniform(-0.1, 0.1)])),
            "m_pitch": f32(R.choice([0.022, 0.022, -0.022, R.uniform(-0.1, 0.1)])),
            "m_side": f32(R.choice([0.25, R.uniform(-1, 1)])),
            "m_forward": f32(R.choice([0.25, R.uniform(-1, 1)])),
        },
    }
    # Aim assist must be initialized whenever CL_MouseMove reaches it: uninitialized, it leaves the melee-charge
    # output unwritten and CL_MouseMove copies stack garbage into the usercmd (records/bugs.tsv), which differs
    # between any two builds by construction.
    st["aa_initialized"] = True if aim_assist == "auto" else aim_assist
    return st


def capped_state():
    """A state where the per-frame speed cap decides the result: real movement, no strafe, mouse look on, a
    small cap and small view angles (so one ulp of the cap shows in the angle)."""
    st = random_state()
    st.update(frame_msec=R.choice([1, 2, 3, 4, 7, 8, 16, 33, R.randint(1, 250)]), pm_flags=0, weaponstate=0,
              strafe=False, mlook=True, mouse_index=0,
              mouse_dx=[R.choice([-1, 1]) * R.randint(20, 3000), 0], mouse_dy=[R.choice([-1, 1]) * R.randint(20, 3000), 0],
              max_yaw=f32(R.uniform(1, 400)), max_pitch=f32(R.uniform(1, 400)),
              viewangles=[f32(R.uniform(-3, 3)), f32(R.uniform(-3, 3)), 0.0])
    st["dvars"]["m_filter"] = False
    return st


def extreme_state():
    """Values the game's dvar limits keep out (NaN, infinity) but the code still has a defined answer for."""
    st = random_state()
    pick = R.choice(["sens", "fov", "angle", "cap", "accel"])
    bad = R.choice([float("nan"), float("inf"), float("-inf")])
    if pick == "sens":
        st["dvars"]["cl_sensitivity"] = bad
    elif pick == "fov":
        st["fov_scale"] = bad
    elif pick == "angle":
        st["viewangles"][R.randrange(2)] = bad
    elif pick == "cap":
        st["max_yaw"] = st["max_pitch"] = bad
    else:
        st["dvars"]["cl_mouseAccel"] = bad
    return st


def setup(fx, st):
    cl = bytearray(CL_SIZE)
    struct.pack_into("<2i", cl, 0x2754, *st["mouse_dx"])
    struct.pack_into("<2i", cl, 0x275C, *st["mouse_dy"])
    struct.pack_into("<i", cl, 0x2764, st["mouse_index"])
    struct.pack_into("<3f", cl, 0x2784, st["fov_scale"], st["max_pitch"], st["max_yaw"])
    struct.pack_into("<3f", cl, 0x2808, *st["viewangles"])
    struct.pack_into("<I", cl, 0x34, st["pm_flags"])
    struct.pack_into("<i", cl, 0x180, st["weaponstate"])
    struct.pack_into("<I", cl, 0x10C, 0x40000 if st["aim_skip"] else 0)
    va = fx.alloc(bytes(cl))
    assert va == CL_VA
    fx.u32(CLIENTS, va)
    fx.i32(CLIENT_UI + 4, st["key_catchers"])
    fx.u32(FRAME_MSEC, st["frame_msec"])
    fx.i32(CLS + 0x120, st["frametime"])
    fx.u8(PLAYERS_KB + KB_STRAFE * KBUTTON_SIZE + 0x10, st["strafe"])
    fx.u8(PLAYERS_KB + KB_MLOOK * KBUTTON_SIZE + 0x10, st["mlook"])
    fx.u8(AA_GLOB + 0x4C, st["aa_initialized"])
    # weapon 0 (aim assist's ps.weapIndex is 0): an all-zero definition, as the fastfiles would load for "none"
    weapon_def = fx.alloc(b"\0" * WEAPON_DEF_SIZE)
    variant = bytearray(WEAPON_VARIANT_DEF_SIZE)
    struct.pack_into("<I", variant, 8, weapon_def)  # WeaponVariantDef.weapDef
    fx.u32(WEAPON_VARIANT_DEFS, fx.alloc(bytes(variant)))
    fx.u32(G_WV + 0x08, fx.win["hwnd"])
    for name, v in st["dvars"].items():
        if isinstance(v, bool):
            make_dvar(fx, DVARS[name], enabled=v)
        else:
            make_dvar(fx, DVARS[name], value=v)


def usercmd():
    cmd = bytearray(R.getrandbits(8) for _ in range(52))
    struct.pack_into("<f", cmd, 0x24, f32(R.uniform(-180, 180)))
    struct.pack_into("<f", cmd, 0x2C, f32(R.uniform(-1, 1)))
    for off in (0x1E, 0x1F, 0x21, 0x22):
        cmd[off] = R.choice([0, 0, 127, 128, R.getrandbits(8)])
    return bytes(cmd)


def aim_input():
    v = [f32(R.uniform(0, 0.1)), angle(), 0.0, 0.0, angle(), 0.0, 0.0, 0.0, 0.0]
    return struct.pack("<9f2Ii", *v, R.getrandbits(32), R.getrandbits(19), 0)


def case(symbol, make_args, ret="void", n=N, **state_kw):
    return {"symbol": symbol, "ret": ret, "setup": setup,
            "inputs": [{"args": make_args(), "state": random_state(**state_kw)} for _ in range(n)]}


CASES = [
    case("?CL_MouseEvent@@YAHHHHH@Z", lambda: [R.randint(0, 1280), R.randint(0, 720), counts(), counts()], ret="int"),
    case("?CL_SetCursorPos@@YAXHH@Z", lambda: [R.randint(-100, 2000), R.randint(-100, 2000)]),
    case("?CL_GetMouseMovement@@YAXPAUclientActive_t@@PAM1@Z", lambda: [CL_VA, b"\0" * 4, b"\0" * 4]),
    {**case("?CL_MouseMove@@YAXHPAUusercmd_s@@@Z", lambda: [0, usercmd()], n=600)},
    case("?CL_SetFOVSensitivityScale@@YAXHM@Z", lambda: [0, ("f", f32(R.choice([1.0, 0.5, R.uniform(0, 2)])))]),
    case("?AimAssist_UpdateMouseInput@@YAXPBUAimInput@@PAUAimOutput@@@Z",
         lambda: [aim_input(), bytes(R.getrandbits(8) for _ in range(16))], aim_assist=False),
    {**case("?AimAssist_UpdateMouseInput@@YAXPBUAimInput@@PAUAimOutput@@@Z",
            lambda: [aim_input(), bytes(R.getrandbits(8) for _ in range(16))], aim_assist=True),
     "symbol": "?AimAssist_UpdateMouseInput@@YAXPBUAimInput@@PAUAimOutput@@@Z"},
]
# run.py keys explicit cases by symbol: give AimAssist_UpdateMouseInput both states in one case
_aa = [c for c in CASES if c["symbol"].startswith("?AimAssist_UpdateMouseInput")]
CASES = [c for c in CASES if not c["symbol"].startswith("?AimAssist_UpdateMouseInput")] + [
    {**_aa[0], "inputs": _aa[0]["inputs"] + _aa[1]["inputs"]}]

# CL_MouseMove: add inputs where the speed cap binds, and out-of-domain values
_mm = next(c for c in CASES if c["symbol"].startswith("?CL_MouseMove"))
_mm["inputs"] += [{"args": [0, usercmd()], "state": capped_state()} for _ in range(600)]
_mm["inputs"] += [{"args": [0, usercmd()], "state": extreme_state()} for _ in range(150)]
