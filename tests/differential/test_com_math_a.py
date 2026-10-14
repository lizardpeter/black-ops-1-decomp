"""Structured differential cases for TRACK_com_math .. PointToLineDistSq (universal/com_math.cpp):
graphs that satisfy GraphGetValueFromFraction's asserts, unit vectors / quaternions for the functions
that assert Vec3IsNormalized (through PerpendicularVector) or Vec4IsNormalized (UnitQuatToAxis), and
in-range inputs for the byte direction table and Q_acos."""
import math
import random
import struct

R = random.Random(0xA0A)  # fixed seed: runs are reproducible
N = 400


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def pack(*v):
    return struct.pack(f"<{len(v)}f", *v)


def unit():
    r = R.random()
    if r < 0.15:
        v = [0.0, 0.0, 0.0]
        v[R.randrange(3)] = R.choice([1.0, -1.0])
        return [R.choice([c, -c]) if c == 0.0 else c for c in v]
    while True:
        v = [R.gauss(0, 1) for _ in range(3)]
        if r < 0.3:  # nearly axis-aligned
            v[R.randrange(3)] *= 1e-4
        n = math.sqrt(sum(c * c for c in v))
        if n > 1e-6:
            return [f32(c / n) for c in v]


def unit_quat():
    if R.random() < 0.15:
        q = [0.0] * 4
        q[R.randrange(4)] = R.choice([1.0, -1.0])
        return pack(*q)
    q = [R.gauss(0, 1) for _ in range(4)]
    n = math.sqrt(sum(c * c for c in q))
    return pack(*(c / n for c in q))


def graph_case():
    count = R.randint(2, 8)
    xs = sorted(R.random() for _ in range(count - 2))
    xs = [0.0] + xs + [1.0]
    if R.random() < 0.2 and count > 2:  # knots on exact binary fractions
        xs = [i / (count - 1) for i in range(count)]
    ys = [R.choice([0.0, 1.0, 0.5]) if R.random() < 0.2 else R.random() for _ in range(count)]
    knots = pack(*(c for x, y in zip(xs, ys) for c in (x, y)))
    k = R.random()
    if k < 0.1:
        frac = R.choice([0.0, 1.0, -0.0])
    elif k < 0.2:
        frac = f32(R.choice(xs))  # exactly on a knot
    else:
        frac = R.random()
    return [count, knots, ("f", frac)]


def degrees():
    return R.choice([0.0, -0.0, 90.0, -90.0, 180.0, 360.0, 1e-30, 1e4, -1e4]) if R.random() < 0.2 \
        else R.uniform(-720.0, 720.0)


def track_args():
    def v():
        return R.choice([0.0, -0.0, 0.001, -0.001, 180.0, -180.0, 360.0]) if R.random() < 0.2 \
            else R.uniform(-400.0, 400.0)
    rate = R.choice([0.0, 1.0, 1e6]) if R.random() < 0.1 else R.uniform(0.0, 50.0)
    dt = R.choice([0.0, 0.05, 1.0]) if R.random() < 0.1 else R.uniform(0.0, 0.2)
    return [("f", v()), ("f", v()), ("f", rate), ("f", dt)]


CASES = [
    {"symbol": "?GraphGetValueFromFraction@@YAMHPAY01$$CBMM@Z", "va": 0x007A8270, "ret": "float",
     "inputs": [graph_case() for _ in range(N)]},
    {"symbol": "?Q_acos@@YAMM@Z", "va": 0x007A85A0, "ret": "float",
     "inputs": [[("f", R.choice([1.0, -1.0, 0.0, -0.0, 1.0000001, -1.0000001, float("nan")])
                  if R.random() < 0.15 else R.uniform(-1.0, 1.0))] for _ in range(N)]},
    {"symbol": "?DirToByte@@YAEQBM@Z", "va": 0x007A8640, "ret": "char",
     "inputs": [[pack(*unit())] for _ in range(N)] + [[0]]},
    {"symbol": "?ByteToDir@@YAXHQAM@Z", "va": 0x007A86F0, "ret": "void",
     "inputs": [[R.randint(-3, 165), pack(1.0, 2.0, 3.0)] for _ in range(N)]},
    {"symbol": "?RotatePointAroundVector@@YAXQAMQBM1M@Z", "va": 0x007A8900, "ret": "void",
     "inputs": [[bytes(12), pack(*unit()), pack(*(R.uniform(-1e3, 1e3) for _ in range(3))), ("f", degrees())]
                for _ in range(N)]},
    {"symbol": "?Vec3Basis_RightHanded@@YAXQBMQAM1@Z", "va": 0x007A8D10, "ret": "void",
     "inputs": [[pack(*unit()), bytes(12), bytes(12)] for _ in range(N)]},
    {"symbol": "?UnitQuatToAngles@@YAXQBMQAM@Z", "va": 0x007A9210, "ret": "void",
     "inputs": [[unit_quat(), bytes(12)] for _ in range(N)]},
    {"symbol": "?PerpendicularVector@@YAXQBMQAM@Z", "va": 0x007A9360, "ret": "void",
     "inputs": [[pack(*unit()), bytes(12)] for _ in range(N)]},
    {"symbol": "?LinearTrack@@YAMMMMM@Z", "va": 0x007A8010, "ret": "float",
     "inputs": [track_args() for _ in range(N)]},
    {"symbol": "?LinearTrackAngle@@YAMMMMM@Z", "va": 0x007A80B0, "ret": "float",
     "inputs": [track_args() for _ in range(N)]},
    {"symbol": "?DiffTrack@@YAMMMMM@Z", "va": 0x007A8150, "ret": "float",
     "inputs": [track_args() for _ in range(N)]},
    {"symbol": "?DiffTrackAngle@@YAMMMMM@Z", "va": 0x007A81D0, "ret": "float",
     "inputs": [track_args() for _ in range(N)]},
]
