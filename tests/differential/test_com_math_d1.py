"""Structured differential inputs for com_math.cpp functions whose auto inputs make the original
assert, dereference garbage, or take an out-of-range x87 path.

Buffers are laid out by harness.Machine.call() at deterministic addresses: argument buffer i lands at
HEAP + i * 0x1000 while each is smaller than 0x1000 - 16 bytes. That lets a `const float *plane[3]`
argument be built as a buffer of addresses pointing at later (extra, ignored-by-cdecl) buffer args.
"""
import math
import random
import struct

R = random.Random(0xD1)
HEAP = 0x00300000
SLOT = 0x1000


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def vec(*xs):
    return struct.pack(f"<{len(xs)}f", *xs)


def unit3():
    while True:
        v = [R.uniform(-1, 1) for _ in range(3)]
        n = math.sqrt(sum(c * c for c in v))
        if n > 1e-3:
            return [f32(c / n) for c in v]


def axis_unit3():
    v = [0.0, 0.0, 0.0]
    v[R.randrange(3)] = R.choice([1.0, -1.0])
    return v


def coord():
    return R.choice([0.0, -0.0, 1.0, -1.0, 0.5, 64.0, -128.0, 1e-3]) if R.random() < 0.2 else R.uniform(-4096, 4096)


def plane_rows(kind):
    """Three planes (normal, dist) of the requested kind."""
    rows = []
    for i in range(3):
        if kind == "axial":
            n = [0.0, 0.0, 0.0]
            n[i] = R.choice([1.0, -1.0])
        elif kind == "parallel" and i == 2:
            n = list(rows[0][:3])  # same normal as plane 0: determinant 0
        elif kind == "nearly_parallel" and i == 2:
            n = [f32(c + R.uniform(-1e-4, 1e-4)) for c in rows[0][:3]]
        else:
            n = unit3()
        rows.append(n + [R.choice([0.0, 8.0, -16.0, 0.125]) if R.random() < 0.2 else f32(R.uniform(-2048, 2048))])
    return rows


def planes_args(kind, lead):
    """[ptrs, *lead, plane0, plane1, plane2]: ptrs points at the three trailing plane buffers."""
    rows = plane_rows(kind)
    first = 1 + len(lead)
    ptrs = struct.pack("<3I", *(HEAP + (first + i) * SLOT for i in range(3)))
    return [ptrs, *lead, *(vec(*r) for r in rows)], rows


def intersect_cases(n):
    out = []
    kinds = ["general"] * 6 + ["axial", "parallel", "nearly_parallel"]
    for _ in range(n):
        args, _rows = planes_args(R.choice(kinds), [vec(0.0, 0.0, 0.0)])
        out.append(args)
    return out


def snap_cases(n):
    out = []
    kinds = ["general"] * 4 + ["axial"] * 3 + ["nearly_parallel"]
    for _ in range(n):
        kind = R.choice(kinds)
        rows = plane_rows(kind)
        grid = R.choice([1.0, 0.5, 0.25, 0.125, 2.0, 8.0, 1.0 / 64])
        # a point near the planes' intersection when it exists, otherwise anywhere
        if R.random() < 0.7:
            base = [R.randint(-512, 512) * grid for _ in range(3)]
            xyz = [f32(c + R.choice([0.0, 1e-4, -1e-4, 3e-3, -0.02, 0.5 * grid, 0.49 * grid])) for c in base]
        else:
            xyz = [f32(coord()) for _ in range(3)]
        eps = R.choice([0.01, 0.001, 0.1, 0.25, 0.0, 1.0])
        first = 2
        ptrs = struct.pack("<3I", *(HEAP + (first + i) * SLOT for i in range(3)))
        out.append([ptrs, vec(*xyz), ("f", grid), ("f", eps), *(vec(*r) for r in rows)])
    return out


def winding_cases(n):
    out = []
    for _ in range(n):
        count = R.choice([0, 1, 2, 3, 3, 4, 4, 5, 6, 8, 12])
        x, y = R.choice([(0, 1), (1, 2), (0, 2), (1, 0), (2, 1), (2, 0)])
        cx, cy, r = R.uniform(-1000, 1000), R.uniform(-1000, 1000), R.uniform(1, 500)
        sign = R.choice([1, -1])
        verts = []
        for k in range(count):
            a = sign * 2 * math.pi * k / max(count, 1) + R.uniform(-0.1, 0.1)
            v = [R.uniform(-100, 100) for _ in range(3)]
            v[x] = cx + r * math.cos(a)
            v[y] = cy + r * math.sin(a)
            verts += v
        p = [R.uniform(-100, 100) for _ in range(3)]
        mode = R.random()
        if mode < 0.4:
            p[x], p[y] = cx + R.uniform(-0.5, 0.5) * r, cy + R.uniform(-0.5, 0.5) * r
        elif mode < 0.6 and count:
            j = R.randrange(count)  # exactly on a vertex
            p[x], p[y] = verts[3 * j + x], verts[3 * j + y]
        else:
            p[x], p[y] = cx + R.uniform(-2, 2) * r, cy + R.uniform(-2, 2) * r
        out.append([vec(*verts) if verts else vec(0.0, 0.0, 0.0), count, x, y, vec(*p)])
    return out


def project_cases(n):
    out = []
    for _ in range(n):
        normal = axis_unit3() if R.random() < 0.2 else unit3()
        out.append([vec(*(coord() for _ in range(3))), vec(*normal), vec(0.0, 0.0, 0.0)])
    return out


def box_cases(n):
    out = []
    for _ in range(n):
        normal = axis_unit3() if R.random() < 0.2 else unit3()
        signbits = sum(1 << j for j in range(3) if normal[j] < 0)
        lo = [coord() for _ in range(3)]
        hi = [c + abs(coord()) for c in lo]
        dist = R.choice([0.0, lo[0], hi[1]]) if R.random() < 0.2 else R.uniform(-6000, 6000)
        plane = vec(*normal, dist) + bytes([R.randrange(6), signbits, 0, 0])
        out.append([vec(*lo), vec(*hi), plane])
    return out


def yaw():
    return R.choice([0.0, -0.0, 90.0, -90.0, 180.0, -180.0, 270.0, 360.0, 720.0, 45.0, 1e-3]) \
        if R.random() < 0.25 else R.uniform(-1080, 1080)


def normal_nonzero():
    r = R.random()
    if r < 0.15:
        return axis_unit3()
    if r < 0.25:
        n = [0.0, 0.0, 0.0]
        n[R.randrange(2)] = R.uniform(-5, 5)  # normal[2] == 0: the 270 path
        return n
    return [R.uniform(-10, 10) for _ in range(3)]


def pitch_cases(n):
    return [[("f", yaw()), vec(*normal_nonzero())] for _ in range(n)]


def nearest_cases(n):
    out = []
    for _ in range(n):
        normal = axis_unit3() if R.random() < 0.2 else unit3()
        out.append([vec(yaw(), yaw(), yaw()), vec(*normal), vec(0.0, 0.0, 0.0)])
    return out


N = 400
CASES = [
    {"symbol": "?IntersectPlanes@@YAHQAPBMQAM@Z", "va": 0x007AE560, "ret": "int", "inputs": intersect_cases(N)},
    {"symbol": "?SnapPointToIntersectingPlanes@@YAXQAPBMQAMMM@Z", "va": 0x007AE930, "ret": "void",
     "inputs": snap_cases(N)},
    {"symbol": "?ProjectedWindingContainsCoplanarPoint@@YAHQAY02$$CBMHHHQBM@Z", "va": 0x007AEBB0, "ret": "int",
     "inputs": winding_cases(N)},
    {"symbol": "?ProjectPointOnPlane@@YAXQBM0QAM@Z", "va": 0x007AEFB0, "ret": "void", "inputs": project_cases(N)},
    {"symbol": "?BoxOnPlaneSide@@YAHQBM0PBUcplane_s@@@Z", "va": 0x007AF140, "ret": "int", "inputs": box_cases(N)},
    {"symbol": "?PitchForYawOnNormal@@YAMMQBM@Z", "va": 0x007AF8E0, "ret": "float", "inputs": pitch_cases(N)},
    {"symbol": "?NearestPitchAndYawOnPlane@@YAXQBM0QAM@Z", "va": 0x007AF9D0, "ret": "void",
     "inputs": nearest_cases(N)},
]
