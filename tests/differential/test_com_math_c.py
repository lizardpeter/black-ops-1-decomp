"""Differential tests for src/universal/com_math.cpp (bounds, projection and axis helpers) that need
structured inputs: unit directions, valid near/far planes, valid bounds, axis/bounds layouts."""
import math
import random
import struct

R = random.Random(0xC0C)  # fixed seed: runs are reproducible
EDGE = [0.0, -0.0, 1.0, -1.0, 0.5, -0.5, 1e-30, -1e-30, 1e30, -1e30, 131072.0, -131072.0,
        float("inf"), float("-inf"), float("nan")]


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def rf(scale=1e4):
    return R.choice(EDGE) if R.random() < 0.2 else R.uniform(-scale, scale)


def pack(*v):
    return struct.pack(f"<{len(v)}f", *v)


def unit():
    """A float unit vector that Vec3IsNormalized accepts (PerpendicularVector asserts on it)."""
    r = R.random()
    if r < 0.15:
        v = [0.0, 0.0, 0.0]
        v[R.randrange(3)] = R.choice([1.0, -1.0])
        return v
    while True:
        v = [R.gauss(0, 1) for _ in range(3)]
        if r < 0.3:  # nearly axis-aligned
            v[R.randrange(3)] *= 1e-4
        n = math.sqrt(sum(c * c for c in v))
        if n > 1e-6:
            return [f32(c / n) for c in v]


def spot_light_view():
    out = []
    for _ in range(200):
        rot = R.choice([0.0, -0.0, math.pi, -math.pi, math.pi / 2, 1e-30, 1e4, -1e4, float("nan")]) \
            if R.random() < 0.2 else R.uniform(-10.0, 10.0)
        out.append([pack(*unit()), ("f", rot), bytes(64)])
    return out


def positive():
    return R.choice([1e-30, 1e-6, 0.001, 1.0, 1e30, float("inf")]) if R.random() < 0.2 \
        else R.uniform(1e-3, 1e4)


def tan_half_fov():
    return R.choice([0.0, -0.0, 1.0, -1.0, 1e-30, 1e30, float("inf"), float("nan")]) if R.random() < 0.2 \
        else R.uniform(0.01, 10.0)


def finite_perspective():
    out = []
    for _ in range(200):
        z_near = positive()
        z_far = z_near + positive() if R.random() < 0.9 else z_near * R.uniform(1.0, 1e6)
        if z_far <= z_near:
            z_far = float("inf")
        out.append([("f", tan_half_fov()), ("f", tan_half_fov()), ("f", z_near), ("f", z_far), bytes(64)])
    return out


def infinite_perspective():
    return [[("f", tan_half_fov()), ("f", tan_half_fov()), ("f", positive()), bytes(64)] for _ in range(200)]


def valid_bounds():
    mins, maxs = [], []
    for _ in range(3):
        a = rf()
        if a != a:
            a = 0.0
        b = a + (R.choice([0.0, -0.0, 1e-30, 1.0, 1e30]) if R.random() < 0.2 else abs(R.uniform(0, 1e4)))
        mins.append(a)
        maxs.append(b)
    return mins, maxs


def expand_bounds_to_width():
    out = []
    for _ in range(250):
        mins, maxs = valid_bounds()
        if R.random() < 0.2:  # equal x and y extents, z either way
            maxs[1] = mins[1] + (maxs[0] - mins[0])
        out.append([pack(*mins), pack(*maxs)])
    return out


def rotated_bounds():
    out = []
    for _ in range(300):
        mins, maxs = valid_bounds()
        if R.random() < 0.3:
            axis = [rf(2.0) for _ in range(9)]  # arbitrary, including -0 / nan sign bits
        else:
            a = [unit(), unit()]
            c = [a[0][1] * a[1][2] - a[0][2] * a[1][1], a[0][2] * a[1][0] - a[0][0] * a[1][2],
                 a[0][0] * a[1][1] - a[0][1] * a[1][0]]
            axis = a[0] + a[1] + c
        out.append([pack(*mins, *maxs), pack(*(rf() for _ in range(3))), pack(*axis), pack(*(rf() for _ in range(6)))])
    return out


# Functions that reach the CRT's floor(): the harness starts with FPCW = MXCSR = 0 (all exceptions unmasked),
# so _floor takes its x87 fallback, which raises a software precision exception when floor() is inexact and
# leaves the emulator unusable. Only inputs whose floor() argument is already integral (or infinite) are
# usable. Under FPCW = 0 the x87 runs at single precision, which is what the filters model.
INV360 = f32(1.0 / 360.0)


def an360_ok(x):
    s = f32(f32(x) * INV360)
    return s == s and (math.isinf(s) or s == math.floor(s))


def an180_ok(x):
    s = f32(f32(x) * INV360)
    if s != s:
        return False
    if math.isinf(s):
        return True
    t = f32(s + 0.5)
    return t == math.floor(t)


def angle_candidates():
    out = [0.0, -0.0, 180.0, -180.0, 360.0, -360.0, 540.0, -540.0, 720.0, float("inf"), float("-inf"),
           1e30, -1e30, 3.4e38, -3.4e38, 2.0 ** 40, -(2.0 ** 40)]
    for _ in range(4000):
        r = R.random()
        if r < 0.4:
            out.append(f32(360.0 * R.randint(-3000, 3000)))
        elif r < 0.8:
            out.append(f32(180.0 + 360.0 * R.randint(-3000, 3000)))
        else:
            out.append(f32(R.uniform(-1, 1) * 10 ** R.uniform(8, 38)))
    return out


ANGLES = angle_candidates()
A360 = [x for x in ANGLES if an360_ok(x)]
A180 = [x for x in ANGLES if an180_ok(x)]


def angle_delta():
    out = []
    while len(out) < 300:
        a = f32(rf()) if R.random() < 0.5 else R.choice(A180)
        d = R.choice(A180)
        b = f32(a - d)
        if a == a and b == b and an180_ok(f32(a - b)):
            out.append([("f", a), ("f", b)])
    return out


def angles_subtract():
    out = []
    while len(out) < 300:
        v1, v2 = [], []
        for _ in range(3):
            while True:
                a = f32(rf()) if R.random() < 0.5 else R.choice(A180)
                b = f32(a - R.choice(A180))
                if a == a and b == b and an180_ok(f32(a - b)):
                    break
            v1.append(a)
            v2.append(b)
        out.append([pack(*v1), pack(*v2), pack(rf(), rf(), rf())])
    return out


CASES = [
    {"symbol": "?AngleNormalize360@@YAMM@Z", "va": 0x007AD2E0, "ret": "float",
     "inputs": [[("f", x)] for x in A360[:300]]},
    {"symbol": "?AngleDelta@@YAMMM@Z", "va": 0x007AD360, "ret": "float",
     "inputs": angle_delta()},
    {"symbol": "?AnglesSubtract@@YAXQBM0QAM@Z", "va": 0x007AD230, "ret": "void",
     "inputs": angles_subtract()},
    {"symbol": "?SpotLightViewMatrix@@YAXQBMMQAY03M@Z", "va": 0x007ACA00, "ret": "void",
     "inputs": spot_light_view()},
    {"symbol": "?FinitePerspectiveMatrix@@YAXMMMMQAY03M@Z", "va": 0x007AC8B0, "ret": "void",
     "inputs": finite_perspective()},
    {"symbol": "?InfinitePerspectiveMatrix@@YAXMMMQAY03M@Z", "va": 0x007ACF20, "ret": "void",
     "inputs": infinite_perspective()},
    {"symbol": "?ExpandBoundsToWidth@@YAXQAM0@Z", "va": 0x007AD610, "ret": "void",
     "inputs": expand_bounds_to_width()},
    {"symbol": "_GetRotatedBounds", "va": 0x007ADD90, "ret": "void",
     "inputs": rotated_bounds()},
]
