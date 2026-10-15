"""Helpers for test setups: original addresses by name, and dvars built in fixture memory.

    from fixtures import sym, static, make_dvar
    G_WV = sym("?g_wv@@3UWinVars_t@@A")            # a global, by its decorated name in the MAP
    S_WMV = static("win32/win_input.cpp", "", "s_wmv")  # a static, from the PDB (statics.tsv)
    make_dvar(fx, sym("?in_mouse@@3PBUdvar_s@@B"), enabled=True)  # a dvar_s, pointer stored in the global
"""
import csv
import functools
import struct

from harness import ROOT, bool_dvar_undefined_bytes

BUILD = "CoDMPServer"
DVAR_SIZE = 0x70        # sizeof(dvar_s) in this build (PDB; 8-byte aligned)
DVAR_CURRENT = 0x18     # offsetof(dvar_s, current)
DVAR_TYPE = 0x10        # offsetof(dvar_s, type)
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


def make_dvar(fx, global_va, enabled=None, value=None, integer=None, string=None):
    """Build a dvar_s in fixture memory with the given current value; store its address in global_va
    (the dvar pointer global). Returns the dvar's address."""
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
    struct.pack_into("<I", d, 0, fx.alloc(_dvar_name(global_va).encode() + b"\0"))  # dvar_s.name (asserted)
    d[DVAR_CURRENT:DVAR_CURRENT + len(cur)] = cur
    va = fx.alloc(bytes(d))
    fx.u32(global_va, va)
    if kind == "bool":
        fx.m.undefined.update(bool_dvar_undefined_bytes(va))
    return va
