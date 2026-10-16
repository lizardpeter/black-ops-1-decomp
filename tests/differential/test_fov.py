"""Differential cases for field of view and view setup: cgame/cg_camera.cpp (CG_UpdateCameraTweenFOV,
CG_GetViewFov, CG_ShouldDefaultViewFov, CG_CalcFov), cgame/cg_main.cpp (CG_GetDefaultFovForView,
CG_UpdateFov, CG_SetThirdPerson), gfx_d3d/r_scene.cpp (R_GetDefaultNearClip, R_SetupProjection,
R_SetupViewProjectionMatrices, R_SetViewParmsForScene) and gfx_d3d/r_init.cpp (R_StoreWindowSettings).

cgame state: local client 0's cg_s and cgs_t built in fixture memory (cgArray/cgsArray point at them), four
weapons (none, an ADS rifle, a three-zoom sniper, a hip-only weapon), four entities and vehicle camera FOVs
for the vehicle view, the demo playback state, and the FOV dvars. Renderer state: the hi-res screenshot
globals, the near-plane dvars, and the window/adapter sizes."""
import math
import random
import struct

from fixtures import make_dvar, sym
from harness import FIXTURE

R = random.Random(0xF0F0C0)  # fixed seed: runs are reproducible
N = 400

CG_ARRAY = sym("?cgArray@@3PAUcg_s@@A")       # cg_s *cgArray (allocated at run time)
CGS_ARRAY = sym("?cgsArray@@3PAUcgs_t@@A")    # cgs_t *cgsArray
ENTITIES = sym("?cg_entitiesArray@@3PAPAUcentity_s@@A")  # centity_s *cg_entitiesArray[1]
VEHICLE_INFOS = sym("?bg_vehicleInfos@@3PAUvehicle_info_t@@A")  # vehicle_info_t bg_vehicleInfos[32]
WEAPON_VARIANT_DEFS = sym("?bg_weaponVariantDefs@@3PAPAUWeaponVariantDef@@A")
LAST_WEAPON = sym("?bg_lastParsedWeaponIndex@@3IA")
DEMO = sym("?demo@@3UdemoMain@@A")
RG = sym("?rg@@3Ur_globals_t@@A")
VIDCONFIG = sym("?vidConfig@@3UvidConfig_t@@A")
DX = sym("?dx@@3UDxGlobals@@A")
DVARS = {n: sym(f"?{n}@@3PBUdvar_s@@B") for n in (
    "cg_fov", "cg_fov_default", "cg_fov_default_thirdperson", "cg_fovScale", "cg_fovMin", "cg_adsZoomToggleStyle",
    "cg_thirdPerson", "r_znear", "r_znear_depthhack", "r_aspectRatio", "com_wideScreen", "r_reflectionProbeGenerate")}
# domains as registered (cgame_mp/cg_main_mp.cpp)
LIMITS = {"cg_fov": (1.0, 160.0), "cg_fov_default": (65.0, 80.0), "cg_fov_default_thirdperson": (1.0, 160.0),
          "cg_fovScale": (0.2, 2.0), "cg_fovMin": (1.0, 160.0), "cg_adsZoomToggleStyle": (0, 1), "cg_thirdPerson": (0, 2)}

# layouts (PDB)
CG_SIZE, CGS_SIZE = 0x71B80, 0x3188
CG_VA = FIXTURE                     # first fixture allocation
PS = 0x4049C                        # cg_s.predictedPlayerState
WEAPON_DEF_SIZE, WEAPON_VARIANT_DEF_SIZE, CENTITY_SIZE, VEHICLE_INFO_SIZE = 2056, 228, 0x328, 0x1DD8
DEMO_STATE, DEMO_PLAYBACK, DEMO_PLAYBACK_INITED, PLAYBACK_CAMERA_MODE = 0x84, 0xB0A54, 0xB0A58, 0x32CC

# weapons 1..3: (aimDownSight, zoom FOVs 1..3, zoom in/out fractions, deployTime)
WEAPONS = [None, (True, 50.0, 40.0, 30.0, 0.5, 0.3, 600), (True, 20.0, 10.0, 6.0, 0.25, 0.9, 1000),
           (False, 65.0, 65.0, 65.0, 1.0, 1.0, 400)]


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def fov():
    return f32(R.choice([65.0, 65.0, 80.0, 90.0, 1.0, 160.0, R.uniform(1, 160)]))


def frac():
    return f32(R.choice([0.0, 0.0, 1.0, 1.0, 0.5, R.random(), R.uniform(-0.2, 1.2)]))


def random_state():
    return {
        "cg_fov": fov(), "cg_fov_default": f32(R.choice([65.0, 80.0, R.uniform(65, 80)])),
        "cg_fov_default_thirdperson": f32(R.choice([40.0, R.uniform(1, 160)])),
        "cg_fovScale": f32(R.choice([1.0, 1.0, 1.0, 0.2, 2.0, R.uniform(0.2, 2)])),
        "cg_fovMin": f32(R.choice([10.0, 10.0, 1.0, 60.0, R.uniform(1, 160)])),
        "cg_adsZoomToggleStyle": R.choice([0, 1]), "cg_thirdPerson": R.choice([0, 0, 1, 2]),
        "weapon": R.randrange(4), "melee_weapon": R.randrange(4),
        "weaponstate": R.choice([0, 0, 0, 35, 35, 17, 18, 19, R.randint(0, 40)]),
        "weapon_time": R.choice([0, 100, 300, 600, 1000, R.randint(-100, 1200)]),
        "pos_frac": frac(), "to_ads": R.random() < 0.5,
        "ads_select": R.choice([0, 1, 2, 3, 4, 5, -1, -2, -3, R.randint(-10, 10)]),
        "other_flags": R.choice([0, 0, 0, 2, R.getrandbits(8)]),
        "eflags": R.choice([0, 0, 0, 0x100, 0x200, 0x4000, 0x4000, 0x4300, R.getrandbits(16) & 0x4300]),
        "viewlocked_ent": R.randrange(4), "next_valid": R.random() < 0.7, "vehicle_index": R.randrange(4),
        "camera_fovs": [f32(R.choice([0.0, 30.0, 65.0, 90.0, R.uniform(0, 160)])) for _ in range(4)],
        "camera_mode": R.choice([0, 0, 0, 1]), "third_person": R.choice([0, 0, 1, 2]),
        "demo": R.choice([None, None, None, 0, 1, 2]),  # None: no demo; else playing with that camera mode
        "time": R.choice([0, 1000, 50000, R.randint(0, 1 << 20)]),
        "tween_start_time": R.choice([0, 0, 900, 49000, 60000, R.randint(0, 1 << 20)]),
        "tween_duration": f32(R.choice([0.0, 0.0, 0.5, 1.0, 2.0, R.uniform(0, 5), -1.0])),
        "tween_start_fov": fov(),
        "view_aspect": f32(R.choice([4 / 3, 16 / 10, 16 / 9, R.uniform(0.5, 4)])),
        # a wider cg_thirdPerson domain now and then, so CG_SetThirdPerson can be given a negative value
        "third_person_limits": R.choice([(0, 2), (0, 2), (-2, 2)]),
    }


def extreme_state():
    """Values the dvar limits keep out (NaN, infinity, zero scale) but the code still has a defined answer for."""
    st = random_state()
    bad = R.choice([float("nan"), float("inf"), float("-inf"), 0.0, -1.0])
    key = R.choice(["cg_fovScale", "cg_fovMin", "pos_frac", "tween_duration", "view_aspect", "tween_start_fov"])
    st[key] = bad
    return st


def deploy_state():
    """Deploying a weapon in first person with nothing else in the way, so the deploy FOV blend decides the
    result (and a cg_fovMin low enough not to hide it)."""
    st = random_state()
    st.update(weaponstate=35, weapon=R.randint(1, 3), other_flags=0, eflags=0, camera_mode=0, demo=None,
              cg_fovMin=f32(R.uniform(1, 10)), cg_fovScale=1.0)
    st["weapon_time"] = R.randint(0, WEAPONS[st["weapon"]][6])
    return st


def weapon_setup(fx):
    fx.u32(LAST_WEAPON, len(WEAPONS) - 1)
    for i, w in enumerate(WEAPONS):
        wdef = bytearray(WEAPON_DEF_SIZE)
        variant = bytearray(WEAPON_VARIANT_DEF_SIZE)
        if w:
            ads, z1, z2, z3, zin, zout, deploy = w
            wdef[0x553] = ads
            struct.pack_into("<i", wdef, 0x44C, deploy)
            struct.pack_into("<5f", variant, 0x64, z1, z2, z3, zin, zout)
        struct.pack_into("<I", variant, 8, fx.alloc(bytes(wdef)))  # WeaponVariantDef.weapDef
        fx.u32(WEAPON_VARIANT_DEFS + 4 * i, fx.alloc(bytes(variant)))


def setup(fx, st):
    cg = bytearray(CG_SIZE)
    struct.pack_into("<i", cg, 0x14, st["camera_mode"])
    struct.pack_into("<i", cg, 0x40488, st["time"])
    struct.pack_into("<i", cg, 0x40498, st["third_person"])
    ps = PS
    struct.pack_into("<i", cg, ps + 0x14, st["other_flags"])
    struct.pack_into("<i", cg, ps + 0x3C, st["weapon_time"])
    struct.pack_into("<I", cg, ps + 0xE0, st["eflags"])
    struct.pack_into("<H", cg, ps + 0x144, st["weapon"])
    struct.pack_into("<I", cg, ps + 0x154, st["melee_weapon"])
    struct.pack_into("<i", cg, ps + 0x158, st["weaponstate"])
    struct.pack_into("<f", cg, ps + 0x168, st["pos_frac"])
    struct.pack_into("<h", cg, ps + 0x440, st["viewlocked_ent"])
    struct.pack_into("<i", cg, ps + 0x444, 5)  # vehiclePos: not a vehicle weapon seat (that needs vehicle entities)
    struct.pack_into("<i", cg, ps + 0x558, st["ads_select"])
    struct.pack_into("<i", cg, 0x42E6C + 4, st["to_ads"])  # playerEntity.bPositionToADS
    struct.pack_into("<f", cg, 0x5B3DC + 0x48, st["tween_start_fov"])
    struct.pack_into("<i", cg, 0x5B3DC + 0x4C, st["tween_start_time"])
    struct.pack_into("<f", cg, 0x5B3DC + 0x50, st["tween_duration"])
    va = fx.alloc(bytes(cg))
    assert va == CG_VA
    fx.u32(CG_ARRAY, va)
    cgs = bytearray(CGS_SIZE)
    struct.pack_into("<f", cgs, 0x10, st["view_aspect"])
    fx.u32(CGS_ARRAY, fx.alloc(bytes(cgs)))
    weapon_setup(fx)
    ents = bytearray(4 * CENTITY_SIZE)
    for i in range(4):
        ents[i * CENTITY_SIZE + 0x270] = st["vehicle_index"]
        struct.pack_into("<I", ents, i * CENTITY_SIZE + 0x324, 2 if st["next_valid"] else 0)
    fx.u32(ENTITIES, fx.alloc(bytes(ents)))
    for i, v in enumerate(st["camera_fovs"]):
        fx.f32(VEHICLE_INFOS + i * VEHICLE_INFO_SIZE + 0xAC, v)
    if st["demo"] is not None:
        fx.i32(DEMO + DEMO_STATE, 2)
        fx.u8(DEMO + DEMO_PLAYBACK_INITED, 1)
        playback = bytearray(0x3300)
        struct.pack_into("<i", playback, PLAYBACK_CAMERA_MODE, st["demo"])
        fx.u32(DEMO + DEMO_PLAYBACK, fx.alloc(bytes(playback)))
    for name in ("cg_fov", "cg_fov_default", "cg_fov_default_thirdperson", "cg_fovScale", "cg_fovMin"):
        make_dvar(fx, DVARS[name], value=st[name], limits=LIMITS[name])
    make_dvar(fx, DVARS["cg_adsZoomToggleStyle"], integer=st["cg_adsZoomToggleStyle"],
              limits=LIMITS["cg_adsZoomToggleStyle"])
    make_dvar(fx, DVARS["cg_thirdPerson"], integer=st["cg_thirdPerson"], limits=st["third_person_limits"])
    # CG_SetThirdPerson sets it by name (cgame_mp registers it; otherwise Dvar_SetBoolByName would register it)
    make_dvar(fx, None, enabled=st["third_person"] > 0, name="cg_spectateThirdPerson", findable=True)


# ---- renderer ---------------------------------------------------------------------------------------------
def unit_axis():
    """A rotation as the game builds one (AnglesToAxis-like), or a degenerate one."""
    if R.random() < 0.1:
        return [f32(R.uniform(-2, 2)) for _ in range(9)]
    y, p = math.radians(R.uniform(-180, 180)), math.radians(R.uniform(-89, 89))
    fwd = [math.cos(p) * math.cos(y), math.cos(p) * math.sin(y), -math.sin(p)]
    left = [-math.sin(y), math.cos(y), 0.0]
    up = [fwd[1] * left[2] - fwd[2] * left[1], fwd[2] * left[0] - fwd[0] * left[2], fwd[0] * left[1] - fwd[1] * left[0]]
    return [f32(v) for v in fwd + left + up]


# bits of a signaling NaN (Python floats can't carry one). A real CPU quiets it in an x87 copy; the emulator
# doesn't (docs/verification.md), so these inputs check NaN propagation, not the x87 copies.
SNAN = 0x7FA00000


def bits(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


def coord(x, p):
    """x as float bits, or a signaling NaN with probability p."""
    return SNAN if R.random() < p else bits(x)


def refdef():
    b = bytearray(0x60)
    tan_y = f32(math.tan(math.radians(R.choice([65.0, 80.0, 30.0, R.uniform(1, 160)])) / 2) * 0.75)
    tan_x = f32(tan_y * R.choice([4 / 3, 16 / 10, 16 / 9, R.uniform(0.5, 4)]))
    struct.pack_into("<3f", b, 0x10, tan_x, tan_y, 0.0)
    struct.pack_into("<3I", b, 0x1C, *[coord(R.choice([0.0, R.uniform(-1e4, 1e4)]), 0.05) for _ in range(3)])
    struct.pack_into("<9I", b, 0x2C, *[coord(v, 0.03) for v in unit_axis()])
    struct.pack_into("<f", b, 0x54, f32(R.choice([0.0, 0.0, -1.0, 0.5, 1.0, 4.0, R.uniform(0, 20)])))
    return bytes(b)


def render_state():
    return {
        "r_znear": f32(R.choice([4.0, 4.0, 1.0, 0.5, 10.0, R.uniform(0, 100)])),
        "r_znear_depthhack": f32(R.choice([0.1, R.uniform(0.001, 16)])),
        "hires": R.random() < 0.3, "tiles": R.choice([0, 1, 2, 4, -1]),
        "row": f32(R.choice([0.0, 1.0, 3.0, R.uniform(0, 4)])), "col": f32(R.choice([0.0, 1.0, 3.0, R.uniform(0, 4)])),
    }


def render_setup(fx, st):
    make_dvar(fx, DVARS["r_znear"], value=st["r_znear"])
    make_dvar(fx, DVARS["r_znear_depthhack"], value=st["r_znear_depthhack"])
    fx.i32(RG + 0x10924, int(st["hires"]))
    fx.i32(RG + 0x10928, st["tiles"])
    fx.f32(RG + 0x1092C, st["row"])
    fx.f32(RG + 0x10930, st["col"])


def view_parms():
    """A GfxViewParms with a projection already in it (R_SetupViewProjectionMatrices reads it)."""
    b = bytearray(R.getrandbits(8) for _ in range(0x140))
    struct.pack_into("<16f", b, 0x00, *[f32(R.uniform(-2, 2)) for _ in range(12)], 0.0, 0.0, 0.0, 1.0)
    tx, ty, zn = f32(R.uniform(0.2, 3)), f32(R.uniform(0.2, 3)), f32(R.uniform(1, 10))
    k = 0.99951171875
    struct.pack_into("<16f", b, 0x40, f32(k / tx), 0, 0, 0, 0, f32(k / ty), 0, 0, 0, 0, k, 1.0, 0, 0, f32(-zn * k), 0)
    struct.pack_into("<f", b, 0x138, zn)  # zNear (InfinitePerspectiveMatrix asserts zNear > 0)
    return bytes(b)


# ---- window settings ----------------------------------------------------------------------------------------
MODES = [(640, 480), (800, 600), (1024, 768), (1280, 1024), (1280, 720), (1280, 800), (1366, 768), (1440, 900),
         (1600, 900), (1680, 1050), (1920, 1080), (1920, 1200), (2160, 1440), (2560, 1080), (2560, 1440),
         (3440, 1440), (3840, 1080), (3840, 2160), (5120, 1440), (1080, 1920), (1, 1)]


def window_state():
    native = R.choice(MODES)
    return {"r_aspectRatio": R.choice([0, 0, 0, 0, 1, 2, 3]), "native_valid": R.random() < 0.7, "native": native,
            "fullscreen_size": R.choice([native, R.choice(MODES)]), "probe": R.choice([None, False, True])}


def window_parms():
    w, h = R.choice(MODES)
    sw, sh = R.choice([(w, h), R.choice(MODES)])
    return struct.pack("<IiB3x6ii", 0x1234, R.choice([60, 75, 120, 144]), R.random() < 0.5, 0, 0, sw, sh, w, h,
                       R.choice([1, 2, 4]))


def window_setup(fx, st):
    make_dvar(fx, DVARS["r_aspectRatio"], integer=st["r_aspectRatio"])
    make_dvar(fx, DVARS["com_wideScreen"], enabled=False)
    if st["probe"] is None:
        fx.u32(DVARS["r_reflectionProbeGenerate"], 0)
    else:
        make_dvar(fx, DVARS["r_reflectionProbeGenerate"], enabled=st["probe"])
    fx.u8(DX + 0x14, st["native_valid"])
    fx.poke(DX + 0x18, struct.pack("<4i", *st["native"], *st["fullscreen_size"]))


def case(symbol, make_args, ret="void", n=N, setup_fn=setup, state_fn=random_state):
    return {"symbol": symbol, "ret": ret, "setup": setup_fn,
            "inputs": [{"args": make_args(), "state": state_fn()} for _ in range(n)]}


def calc_fov_arg():
    return ("f", f32(R.choice([-1.0, -1.0, 0.0, 65.0, 40.0, 90.0, R.uniform(1, 160)])))


CASES = [
    case("?CG_UpdateCameraTweenFOV@@YAMHM@Z", lambda: [0, ("f", fov())], ret="float"),
    case("?CG_GetViewFov@@YAMH@Z", lambda: [0], ret="float", n=1500),
    case("?CG_ShouldDefaultViewFov@@YA_NH@Z", lambda: [0], ret="bool"),
    case("?CG_CalcFov@@YAXHM@Z", lambda: [0, calc_fov_arg()], n=1200),
    case("?CG_GetDefaultFovForView@@YAMXZ", lambda: [], ret="float"),
    case("?CG_UpdateFov@@YAXM@Z", lambda: [("f", R.choice([65.0, fov()]))]),
    case("?CG_SetThirdPerson@@YAXH@Z", lambda: [R.choice([0, 0, 1, 2, -1])]),
    case("?R_GetDefaultNearClip@@YAMXZ", lambda: [], ret="float", setup_fn=render_setup, state_fn=render_state),
    case("?R_SetupProjection@@YAXMMPAUGfxViewParms@@@Z",
         lambda: [("f", f32(R.uniform(0.1, 4))), ("f", f32(R.uniform(0.1, 4))), view_parms()],
         setup_fn=render_setup, state_fn=render_state),
    case("?R_SetupViewProjectionMatrices@@YAXPAUGfxViewParms@@_N@Z", lambda: [view_parms(), R.choice([0, 1])],
         setup_fn=render_setup, state_fn=render_state),
    case("?R_SetViewParmsForScene@@YAXPBUrefdef_s@@PAUGfxViewParms@@@Z",
         lambda: [refdef(), bytes(R.getrandbits(8) for _ in range(0x140))], setup_fn=render_setup,
         state_fn=render_state),
    case("?R_StoreWindowSettings@@YAXPBUGfxWindowParms@@@Z", lambda: [window_parms()], n=600,
         setup_fn=window_setup, state_fn=window_state),
]

# CG_GetViewFov: add states where the deploy blend decides the result
next(c for c in CASES if c["symbol"].startswith("?CG_GetViewFov"))["inputs"] += [
    {"args": [0], "state": deploy_state()} for _ in range(400)]

# CG_GetViewFov and CG_CalcFov: add out-of-domain values
for _c in CASES:
    if _c["symbol"].startswith(("?CG_GetViewFov", "?CG_CalcFov")):
        _c["inputs"] += [{"args": [0] + ([calc_fov_arg()] if "CalcFov" in _c["symbol"] else []),
                          "state": extreme_state()} for _ in range(300)]
