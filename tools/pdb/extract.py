#!/usr/bin/env python3
"""Extract structured tables from an original PDB (+ linker MAP) into analysis/<build>/.

Generated output only. Re-running on the same PDB/MAP reproduces identical files.

    python3 tools/pdb/extract.py <build-id> <exe> <pdb> <map>

Needs llvm-pdbutil (apt install llvm).

Writes:
  analysis/<build>/pdb/sections.tsv    section index, name, virtual address, virtual size
  analysis/<build>/pdb/modules.tsv     one row per compiland: index, obj, primary source file
  analysis/<build>/pdb/functions.tsv   every function the PDB knows: va, size, name, scope, module, source, line, type
  analysis/<build>/pdb/lines.tsv       function start -> (source file, first line) from line tables
  analysis/<build>/pdb/raw/*.txt       llvm-pdbutil dumps the tables were parsed from
  analysis/<build>/map/publics.tsv     linker MAP publics: va, name, flags, lib, obj
"""
import hashlib
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC_PREFIXES = [r"c:\projects_pc\cod\codsrc\src" + "\\"]


def rel_source(path: str) -> str:
    """Original absolute Windows path -> repo-style relative path ('game_mp/pregame.cpp')."""
    if not path:
        return ""
    low = path.lower()
    for pre in SRC_PREFIXES:
        if low.startswith(pre):
            return path[len(pre):].replace("\\", "/")
    # outside the game tree (CRT, SDKs): keep it recognisable but drive-less
    p = re.sub(r"^[a-zA-Z]:\\", "", path)
    return "<external>/" + p.replace("\\", "/")


def pdbutil(*args) -> str:
    return subprocess.run(["llvm-pdbutil", *args], check=True, capture_output=True, text=True,
                          errors="replace").stdout


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def tsv(path: Path, header, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", newline="\n") as f:
        f.write("\t".join(header) + "\n")
        for r in rows:
            f.write("\t".join("" if v is None else str(v).replace("\t", " ").replace("\n", " ") for v in r) + "\n")


def parse_sections(text):
    secs, cur = [], None
    for line in text.splitlines():
        m = re.match(r"\s*SECTION HEADER #(\d+)", line)
        if m:
            cur = {"index": int(m.group(1))}
            secs.append(cur)
            continue
        if cur is None:
            continue
        m = re.match(r"\s*(\S+) name$", line)
        if m:
            cur["name"] = m.group(1)
        m = re.match(r"\s*([0-9A-Fa-f]+) virtual size$", line)
        if m:
            cur["vsize"] = int(m.group(1), 16)
        m = re.match(r"\s*([0-9A-Fa-f]+) virtual address$", line)
        if m:
            cur["rva"] = int(m.group(1), 16)
    return secs


def parse_modules(text):
    mods, cur = {}, None
    for line in text.splitlines():
        m = re.match(r"\s*Mod (\d{4}) \| `(.*)`:", line)
        if m:
            cur = {"index": int(m.group(1)), "obj": m.group(2), "lib": "", "source": ""}
            mods[cur["index"]] = cur
            continue
        if cur is None:
            continue
        m = re.match(r"\s*Obj: `(.*)`:", line)
        if m:
            cur["lib"] = m.group(1) if m.group(1) != cur["obj"] else ""
        m = re.search(r"src file ni: \d+ `(.*)`", line)
        if m:
            cur["source"] = m.group(1)
    return mods


def parse_procs(text, mods, va_of):
    out, mod = [], None
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        m = re.match(r"\s*Mod (\d{4}) \|", line)
        if m:
            mod = int(m.group(1))
        m = re.match(r"\s*\d+ \| (S_GPROC32|S_LPROC32|S_THUNK32) \[size = \d+\] `(.*)`\s*$", line)
        if m:
            kind, name = m.group(1), m.group(2)
            body = " ".join(lines[i + 1:i + 3])
            a = re.search(r"addr = (\d{4}):(\d+)", body)
            sz = re.search(r"(?:code size|len) = (\d+)", body)
            ty = re.search(r"type = `0x[0-9A-F]+ \((.*)\)`", body)
            if a:
                va = va_of(int(a.group(1)), int(a.group(2)))
                scope = {"S_GPROC32": "global", "S_LPROC32": "local", "S_THUNK32": "thunk"}[kind]
                src = mods.get(mod, {}).get("source", "")
                out.append([va, int(sz.group(1)) if sz else "", name, scope, mod, rel_source(src),
                            ty.group(1) if ty else ""])
        i += 1
    return out


def parse_lines(text, va_of):
    """First line of each line-table block, keyed by its start address."""
    out, src = {}, ""
    pending = None
    for line in text.splitlines():
        m = re.match(r"^([a-zA-Z]:\\.*) \(MD5:", line)
        if m:
            src = m.group(1)
            continue
        m = re.match(r"\s*(\d{4}):([0-9A-F]{8})-[0-9A-F]{8}, line/addr entries", line)
        if m:
            pending = va_of(int(m.group(1)), int(m.group(2), 16))
            continue
        if pending is not None:
            m = re.match(r"\s*(\d+) [0-9A-F]{8}", line)
            if m:
                out.setdefault(pending, (rel_source(src), int(m.group(1))))
                pending = None
    return out


def parse_map(text):
    rows = []
    pat = re.compile(r"^ ([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})((?: [fi])*)\s+(\S.*)$")
    for line in text.splitlines():
        m = pat.match(line)
        if not m:
            continue
        va = int(m.group(4), 16)
        if va == 0:
            continue  # absolute / imported placeholder rows
        flags = m.group(5).split()
        where = m.group(6).strip()
        lib, _, obj = where.rpartition(":")
        rows.append([va, m.group(3), "".join(flags), lib, obj])
    return rows


def main():
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    build, exe, pdb, mapf = sys.argv[1], Path(sys.argv[2]), Path(sys.argv[3]), Path(sys.argv[4])
    out = ROOT / "analysis" / build
    raw = out / "pdb" / "raw"
    raw.mkdir(parents=True, exist_ok=True)

    dumps = {
        "summary": pdbutil("dump", "-summary", str(pdb)),
        "sections": pdbutil("dump", "-section-headers", str(pdb)),
        "modules": pdbutil("dump", "-modules", str(pdb)),
        "symbols": pdbutil("dump", "-symbols", str(pdb)),
        "publics": pdbutil("dump", "-publics", str(pdb)),
        "lines": pdbutil("dump", "-l", str(pdb)),
    }
    for k, v in dumps.items():
        (raw / f"{k}.txt").write_text(v)

    secs = parse_sections(dumps["sections"])
    base = 0x400000
    rva = {s["index"]: s["rva"] for s in secs}

    def va_of(seg, off):
        return f"{base + rva[seg] + off:08X}"

    tsv(out / "pdb" / "sections.tsv", ["index", "name", "va", "vsize"],
        [[s["index"], s.get("name", ""), f"{base + s['rva']:08X}", f"{s.get('vsize', 0):X}"] for s in secs])

    mods = parse_modules(dumps["modules"])
    tsv(out / "pdb" / "modules.tsv", ["module", "obj", "lib", "source"],
        [[m["index"], rel_source(m["obj"]), rel_source(m["lib"]), rel_source(m["source"])]
         for m in sorted(mods.values(), key=lambda m: m["index"])])

    first_line = parse_lines(dumps["lines"], va_of)
    procs = parse_procs(dumps["symbols"], mods, va_of)
    procs.sort(key=lambda r: (r[0], r[2]))
    tsv(out / "pdb" / "functions.tsv", ["va", "size", "name", "scope", "module", "module_source", "line_source", "line", "type"],
        [[p[0], p[1], p[2], p[3], p[4], p[5], *first_line.get(p[0], ("", "")), p[6]] for p in procs])
    tsv(out / "pdb" / "lines.tsv", ["va", "source", "line"],
        [[va, s, ln] for va, (s, ln) in sorted(first_line.items())])

    map_text = mapf.read_text(errors="replace")
    (out / "map").mkdir(parents=True, exist_ok=True)
    tsv(out / "map" / "publics.tsv", ["va", "name", "flags", "lib", "obj"],
        [[f"{r[0]:08X}", *r[1:]] for r in sorted(parse_map(map_text), key=lambda r: (r[0], r[1]))])

    (out / "inputs.tsv").write_text(
        "role\tfile\tsha256\n" + "".join(f"{role}\t{p.name}\t{sha256(p)}\n" for role, p in
                                          (("exe", exe), ("pdb", pdb), ("map", mapf))))
    print(f"{build}: {len(mods)} modules, {len(procs)} PDB procs, {len(first_line)} line blocks")


if __name__ == "__main__":
    main()
