"""Helpers for test setups: original addresses by name, and dvars built in fixture memory.

    from fixtures import sym, static, make_dvar
    G_WV = sym("?g_wv@@3UWinVars_t@@A")            # a global, by its decorated name in the MAP
    S_WMV = static("win32/win_input.cpp", "", "s_wmv")  # a static, from the PDB (statics.tsv)
    make_dvar(fx, sym("?in_mouse@@3PBUdvar_s@@B"), enabled=True)  # a dvar_s, pointer stored in the global
"""
import csv
import functools
import struct

from harness import ROOT, dvar_undefined_bytes

BUILD = "CoDMPServer"
DVAR_SIZE = 0x70        # sizeof(dvar_s) in this build (PDB; 8-byte aligned)
DVAR_CURRENT = 0x18     # offsetof(dvar_s, current)
DVAR_TYPE = 0x10        # offsetof(dvar_s, type)
DVAR_LIMITS = 0x58      # offsetof(dvar_s, domain)
DVAR_HASH = 0x08        # offsetof(dvar_s, hash): Com_HashString of the name
DVAR_HASH_NEXT = 0x68   # offsetof(dvar_s, hashNext)
DVAR_TYPES = {"bool": 0, "float": 1, "int": 5, "string": 7}


def _tsv(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE))


@functools.cache
def _publics():
    out = {}
    for r in _tsv(ROOT / "analysis" / BUILD / "map" / "publics.tsv"):
        out.setdefault(r["name"], int(r["va"], 16))
    return out


@functools.cache
def _statics():
    return {(r["source"], r["scope"], r["name"]): int(r["va"], 16)
            for r in _tsv(ROOT / "analysis" / BUILD / "pdb" / "statics.tsv")}


@functools.cache
def _names_by_va():
    return {va: n for n, va in _publics().items()}


def _dvar_name(global_va):
    """The dvar's name from its pointer global's decorated name: ?in_mouse@@3PBUdvar_s@@B -> in_mouse."""
    n = _names_by_va().get(global_va, "?dvar@@")
    return n[1:n.index("@@")] if n.startswith("?") else n


def sym(decorated):
    return _publics()[decorated]


def static(source, scope, name):
    return _statics()[(source, scope, name)]


def dvar_hash(name):
    """Com_HashString(name, 0) (qcommon): djb2 over the lowercased name, 32-bit signed."""
    h = 0x1505
    for c in name.lower().encode():
        h = (h * 33 + c) & 0xFFFFFFFF
    return h - (1 << 32) if h >= 1 << 31 else h


def make_dvar(fx, global_va, enabled=None, value=None, integer=None, string=None, limits=None, name=None,
              findable=False):
    """Build a dvar_s in fixture memory with the given current value; store its address in global_va
    (the dvar pointer global; None for a dvar only reached by name). limits=(min, max) sets the domain of a
    float or int dvar, as its registration would (Dvar_Set* assert that a new value is inside it).
    findable=True links it into dvarHashTable under its name, so Dvar_FindVar("name") finds it.
    Returns the dvar's address."""
    d = bytearray(DVAR_SIZE)
    if enabled is not None:
        kind, cur = "bool", bytes([1 if enabled else 0])
    elif value is not None:
        kind, cur = "float", struct.pack("<f", value)
    elif string is not None:
        kind, cur = "string", struct.pack("<I", fx.alloc(string.encode() + b"\0"))
    else:
        kind, cur = "int", struct.pack("<i", integer or 0)
    struct.pack_into("<I", d, DVAR_TYPE, DVAR_TYPES[kind])
    name = name or _dvar_name(global_va)
    struct.pack_into("<I", d, 0, fx.alloc(name.encode() + b"\0"))  # dvar_s.name (asserted)
    struct.pack_into("<i", d, DVAR_HASH, dvar_hash(name))
    d[DVAR_CURRENT:DVAR_CURRENT + len(cur)] = cur
    if limits is not None:
        struct.pack_into("<2f" if kind == "float" else "<2i", d, DVAR_LIMITS, *limits)
    va = fx.alloc(bytes(d))
    if global_va is not None:
        fx.u32(global_va, va)
    if findable:
        bucket = static("universal/dvar.cpp", "", "dvarHashTable") + 4 * (dvar_hash(name) & 0x3FF)
        fx.poke(va + DVAR_HASH_NEXT, bytes(fx.m.mu.mem_read(bucket, 4)))  # chain in front of what is there
        fx.u32(bucket, va)
    if kind in ("bool", "int", "float"):
        fx.m.undefined.update(dvar_undefined_bytes(va, 1 if kind == "bool" else 4))
    return va
