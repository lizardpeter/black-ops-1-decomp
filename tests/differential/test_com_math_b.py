"""Differential tests for src/universal/com_math.cpp (slice b) against build CoDMPServer.

Hand-written where auto inputs are meaningless for the original: plane counts must fit the buffer,
quaternions must be unit length (the original asserts Vec4IsNormalized), matrices should be invertible.
"""
import math
import random
import struct

R = random.Random(0xB0B)
SPECIAL_F = [0.0, -0.0, 1.0, -1.0, 0.5, -0.5, 1e-30, -1e-30, 131072.0, -131072.0,
             float("inf"), float("-inf"), float("nan")]


def rf():
    r = R.random()
    if r < 0.15:
        return R.choice(SPECIAL_F)
    if r < 0.6:
        return R.uniform(-2.0, 2.0)
    return R.uniform(-1e4, 1e4)


def fv(n, gen=rf):
    return struct.pack(f"<{n}f", *(gen() for _ in range(n)))


def planes(n):
    out = b""
    for _ in range(n):
        x, y, z = R.gauss(0, 1), R.gauss(0, 1), R.gauss(0, 1)
        l = math.sqrt(x * x + y * y + z * z) or 1.0
        if R.random() < 0.1:
            out += fv(4)
        else:
            out += struct.pack("<4f", x / l, y / l, z / l, R.uniform(-200, 200))
    return out


def box():
    c = [R.uniform(-150, 150) for _ in range(3)]
    e = [abs(R.uniform(0, 80)) for _ in range(3)]
    if R.random() < 0.05:
        return fv(3), fv(3)
    return (struct.pack("<3f", *(c[i] - e[i] for i in range(3))),
            struct.pack("<3f", *(c[i] + e[i] for i in range(3))))


def unit_quat():
    q = [R.gauss(0, 1) for _ in range(4)]
    l = math.sqrt(sum(v * v for v in q))
    q = [v / l for v in q]
    if R.random() < 0.15:  # axis-aligned and other exact unit quaternions
        q = [0.0] * 4
        q[R.randrange(4)] = R.choice([1.0, -1.0])
    return struct.pack("<4f", *q)


N = 300
box_inputs = []
for _ in range(N):
    n = R.randint(0, 6)
    mins, maxs = box()
    box_inputs.append([planes(n), n, mins, maxs])

sphere_inputs = []
for _ in range(N):
    n = R.randint(0, 6)
    sphere_inputs.append([planes(n), n, struct.pack("<3f", *(R.uniform(-200, 200) for _ in range(3))),
                          ("f", R.choice([0.0, -0.0, 1.0, 50.0, float("inf"), float("nan")])
                           if R.random() < 0.1 else R.uniform(0, 120))])

CASES = [
    {"symbol": "?BoxInPlanes@@YAHPAY03$$CBMIQBM1@Z", "va": 0x007A9780, "ret": "int", "inputs": box_inputs},
    {"symbol": "?SphereInPlanes@@YAHPAY03$$CBMIQBMM@Z", "va": 0x007A9B80, "ret": "int", "inputs": sphere_inputs},
    {"symbol": "?UnitQuatToAxis@@YAXQBMQAY02M@Z", "va": 0x007AC120, "ret": "void",
     "inputs": [[unit_quat(), fv(9)] for _ in range(N)]},
    {"symbol": "?UnitQuatToForward@@YAXQBMQAM@Z", "va": 0x007AC350, "ret": "void",
     "inputs": [[unit_quat(), fv(3)] for _ in range(N)]},
    # QuatSlerp has no assert: unit quaternions for the meaningful path, arbitrary ones for edge values
    {"symbol": "?QuatSlerp@@YAXQBM0MQAM@Z", "va": 0x007AC480, "ret": "void",
     "inputs": [[unit_quat(), unit_quat(), ("f", R.choice([0.0, 1.0, 0.5, -0.25, 1.5]) if R.random() < 0.2
                                             else R.uniform(0, 1)), fv(4)] for _ in range(N)]
               + [[fv(4), fv(4), ("f", rf()), fv(4)] for _ in range(100)]},
]
