"""Differential tests for src/universal/com_math.cpp against build CoDMPServer."""
import random
import struct

R = random.Random(0x75)  # fixed seed: runs are reproducible
SPECIAL_F = [0.0, -0.0, 1.0, -1.0, 0.5, 1e-30, -1e-30, 131072.0, -131072.0, 3.4e38, -3.4e38,
             float("inf"), float("-inf"), float("nan")]
SPECIAL_I = [0, 1, -1, 127, 128, -128, -129, 32767, 32768, -32768, -32769, 2**31 - 1, -2**31]


def rf():
    return R.choice(SPECIAL_F) if R.random() < 0.3 else R.uniform(-1e4, 1e4)


def ri():
    return R.choice(SPECIAL_I) if R.random() < 0.3 else R.randint(-2**31, 2**31 - 1)


def fv(n):
    return struct.pack(f"<{n}f", *(rf() for _ in range(n)))


def plane():
    return fv(4) + bytes([R.randint(0, 255) for _ in range(4)])


N = 400
CASES = [
    {"symbol": "?RandWithSeed@@YAHPAH@Z", "va": 0x007A7F30, "ret": "int",
     "inputs": [[struct.pack("<i", ri())] for _ in range(N)]},
    {"symbol": "?ClampChar@@YACH@Z", "va": 0x007A85F0, "ret": "char",
     "inputs": [[ri()] for _ in range(N)]},
    {"symbol": "?ClampShort@@YAFH@Z", "va": 0x007A8610, "ret": "short",
     "inputs": [[ri()] for _ in range(N)]},
    {"symbol": "?VecNCompareCustomEpsilon@@YAHQBM0MH@Z", "va": 0x007A8770, "ret": "int",
     "inputs": [[fv(4), fv(4), ("f", abs(rf())), R.randint(0, 4)] for _ in range(N)]},
    {"symbol": "_ClearBounds", "va": 0x007AD780, "ret": "void",
     "inputs": [[fv(3), fv(3)] for _ in range(20)]},
    {"symbol": "?ClearBounds2D@@YAXQAM0@Z", "va": 0x007AD880, "ret": "void",
     "inputs": [[fv(2), fv(2)] for _ in range(20)]},
    {"symbol": "?AddPointToBounds2D@@YAXQBMQAM1@Z", "va": 0x007AD980, "ret": "void",
     "inputs": [[fv(2), fv(2), fv(2)] for _ in range(N)]},
    {"symbol": "?ExpandBounds2D@@YAXQBM0QAM1@Z", "va": 0x007ADD10, "ret": "void",
     "inputs": [[fv(2), fv(2), fv(2), fv(2)] for _ in range(N)]},
    {"symbol": "?SetPlaneSignbits@@YAXPAUcplane_s@@@Z", "va": 0x007AF0E0, "ret": "void",
     "inputs": [[plane()] for _ in range(N)]},
]
