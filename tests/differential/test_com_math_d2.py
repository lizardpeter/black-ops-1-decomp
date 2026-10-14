"""Structured differential cases for flrand .. colorSaturationMatrix (universal/com_math.cpp)."""
import math
import random
import struct

R = random.Random(0xD2)
N = 400


def pack(*v):
    return struct.pack(f"<{len(v)}f", *v)


def unit3():
    while True:
        v = [R.uniform(-1, 1) for _ in range(3)]
        n = math.sqrt(sum(x * x for x in v))
        if 1e-3 < n <= 1:
            return [x / n for x in v]


def axis_dir():
    # axis-aligned and signed-zero components exercise the I_fsel branches
    d = [0.0, 0.0, 0.0]
    d[R.randrange(3)] = R.choice([1.0, -1.0])
    return [R.choice([x, -x]) if x == 0.0 else x for x in d]


def rot_from_quat(w, x, y, z):
    return [[1 - 2 * (y * y + z * z), 2 * (x * y + w * z), 2 * (x * z - w * y)],
            [2 * (x * y - w * z), 1 - 2 * (x * x + z * z), 2 * (y * z + w * x)],
            [2 * (x * z + w * y), 2 * (y * z - w * x), 1 - 2 * (x * x + y * y)]]


def rand_quat():
    q = [R.gauss(0, 1) for _ in range(4)]
    n = math.sqrt(sum(c * c for c in q))
    return [c / n for c in q]


def axis_case():
    k = R.random()
    if k < 0.25:  # near-180-degree rotations: forces the best = 1, 2, 3 branches
        a = unit3()
        ang = math.pi - R.uniform(0, 0.3)
        s = math.sin(ang / 2)
        q = [math.cos(ang / 2), a[0] * s, a[1] * s, a[2] * s]
    elif k < 0.35:
        q = R.choice([[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1],
                      [0, 0.70710678, 0.70710678, 0], [0.5, 0.5, 0.5, 0.5]])
    else:
        q = rand_quat()
    m = rot_from_quat(*q)
    return [pack(*(c for row in m for c in row)), pack(0, 0, 0, 0)]


def quat_case():
    qa, qb = rand_quat(), rand_quat()
    if R.random() < 0.1:
        qb = [-c for c in qa]
    frac = R.choice([0.0, 1.0, 0.5]) if R.random() < 0.2 else R.uniform(-0.25, 1.25)
    return [pack(*qa), pack(*qb), ("f", frac), pack(0, 0, 0, 0)]


def cos_half_fov():
    return R.choice([0.0, 1.0, 0.5, 0.70710678, 0.99]) if R.random() < 0.2 else R.uniform(0.0, 1.0)


def pos(scale=1000.0):
    return [R.uniform(-scale, scale) for _ in range(3)]


def half_size():
    return [R.choice([0.0, R.uniform(0, 500)]) if R.random() < 0.1 else R.uniform(0.5, 500) for _ in range(3)]


def cone_dir():
    return axis_dir() if R.random() < 0.15 else unit3()


def cone_box_case():
    org = pos()
    d = cone_dir()
    c = cos_half_fov()
    dist = R.uniform(-500, 2000)
    center = [org[i] + d[i] * dist + R.uniform(-600, 600) for i in range(3)]
    return [pack(*org), pack(*d), ("f", c), pack(*center), pack(*half_size())]


def sphere_box_case():
    org = pos()
    center = [o + R.uniform(-800, 800) for o in org]
    r = R.choice([0.0, 1.0]) if R.random() < 0.05 else R.uniform(0, 800)
    return [pack(*org), ("f", r), pack(*center), pack(*half_size())]


def conic_box_case():
    org = pos()
    d = cone_dir()
    c = cos_half_fov()
    dist = R.uniform(-500, 2000)
    center = [org[i] + d[i] * dist + R.uniform(-600, 600) for i in range(3)]
    r = R.uniform(0, 2500)
    return [pack(*org), pack(*d), ("f", c), ("f", r), pack(*center), pack(*half_size())]


def sphere_cone_case():
    org = pos()
    d = cone_dir()
    c = cos_half_fov()
    dist = R.uniform(-500, 2000)
    center = [org[i] + d[i] * dist + R.uniform(-600, 600) for i in range(3)]
    r = R.uniform(0, 600)
    return [pack(*org), pack(*d), ("f", c), pack(*center), ("f", r)]


TEMP_EDGES = [0.0, -1.0, 1000.0, 1649.9, 1650.0, 1650.1, 2221.9, 2222.0, 2222.1, 3999.9, 4000.0, 4000.1,
              6499.0, 6500.0, 6999.9, 7000.0, 7000.1, 24999.0, 25000.0, 25001.0, 40000.0]


def temp():
    return R.choice(TEMP_EDGES) if R.random() < 0.2 else R.uniform(1000.0, 30000.0)


M44 = pack(*([0.0] * 16))
rand_int_pair = lambda: sorted([R.randint(-100000, 100000), R.randint(-100000, 100000)])

CASES = [
    {"symbol": "?flrand@@YAMMM@Z", "va": 0x007AFA90, "ret": "float",
     "inputs": [[("f", a), ("f", b)] for a, b in
                ([R.uniform(-1000, 1000), R.uniform(-1000, 1000)] for _ in range(N))]
               + [[("f", 0.0), ("f", 1.0)], [("f", -1.0), ("f", 1.0)], [("f", 5.0), ("f", 5.0)]]},
    {"symbol": "?irand@@YAHHH@Z", "va": 0x007AFAF0, "ret": "int",
     "inputs": [rand_int_pair() for _ in range(N)]
               + [[0, 1], [0, 2], [-5, 5], [7, 7], [0, 2**31 - 1], [-2**30, 2**30]]},
    {"symbol": "?AxisToQuat@@YAXQAY02$$CBMQAM@Z", "va": 0x007AFB70, "ret": "void",
     "inputs": [axis_case() for _ in range(N)]},
    {"symbol": "?QuatLerp@@YAXQBM0MQAM@Z", "va": 0x007AFE60, "ret": "void",
     "inputs": [quat_case() for _ in range(N)]},
    {"symbol": "?CullBoxFromCone@@YA_NQBM0M00@Z", "va": 0x007AFF60, "ret": "char",
     "inputs": [cone_box_case() for _ in range(N)]},
    {"symbol": "?CullBoxFromSphere@@YA_NQBMM00@Z", "va": 0x007B02F0, "ret": "char",
     "inputs": [sphere_box_case() for _ in range(N)]},
    {"symbol": "?CullBoxFromConicSectionOfSphere@@YA_NQBM0MM00@Z", "va": 0x007B0490, "ret": "char",
     "inputs": [conic_box_case() for _ in range(N)]},
    {"symbol": "?CullSphereFromCone@@YA_NQBM0M0M@Z", "va": 0x007B09D0, "ret": "char",
     "inputs": [sphere_cone_case() for _ in range(N)]},
    {"symbol": "?colorTempToXYZ@@YAXMQAM@Z", "va": 0x007B1080, "ret": "void",
     "inputs": [[("f", t), pack(0, 0, 0, 0)] for t in TEMP_EDGES]
               + [[("f", temp()), pack(0, 0, 0, 0)] for _ in range(N)]},
    {"symbol": "?colorTempMatrix@@YAXQAY03MM@Z", "va": 0x007B0B70, "ret": "void",
     "inputs": [[M44, ("f", t)] for t in TEMP_EDGES] + [[M44, ("f", temp())] for _ in range(N)]},
    {"symbol": "?colorHueMatrix@@YAXQAY03MM@Z", "va": 0x007B1340, "ret": "void",
     "inputs": [[M44, ("f", h)] for h in [0.0, -0.0, 90.0, 180.0, -180.0, 360.0, 45.0, 120.0, 240.0]]
               + [[M44, ("f", R.uniform(-360.0, 720.0))] for _ in range(N)]},
    {"symbol": "?colorSaturationMatrix@@YAXQAY03MM@Z", "va": 0x007B1A00, "ret": "void",
     "inputs": [[pack(*[R.uniform(-9, 9) for _ in range(16)]), ("f", s)] for s in [0.0, 1.0, 0.5, -1.0, 2.0]]
               + [[pack(*[R.uniform(-9, 9) for _ in range(16)]), ("f", R.uniform(-1.0, 3.0))] for _ in range(N)]},
]
