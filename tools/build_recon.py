#!/usr/bin/env python3
"""Compile every reconstructed source file into one x86 Windows DLL for the differential tests.

    python3 tools/build_recon.py            # -> build/recon.dll, build/recon.symbols.tsv

Uses clang targeting i686-pc-windows-msvc so the calling convention and C++ name mangling are
MSVC's. Every defined external symbol is exported under its decorated name, which lets the tests
look functions up by the ORIGINAL decorated name from the MAP: a wrong prototype simply won't be found.

This is the `compiles` gate. It is not byte matching: that needs the original VC90 compiler.
"""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
CFLAGS = ["--target=i686-pc-windows-msvc", "-O0", "-msse2", "-mfpmath=sse", "-fno-exceptions",
          "-fms-extensions", "-fno-stack-protector", "-ffreestanding", "-Wall", "-Werror", "-Wno-unused-function",
          f"-I{ROOT / 'include'}"]


def main():
    BUILD.mkdir(exist_ok=True)
    objs = []
    for src in sorted((ROOT / "src").rglob("*.cpp")):
        rel = src.relative_to(ROOT / "src")
        obj = BUILD / "obj" / rel.with_suffix(".obj")
        obj.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(["clang++", "-std=c++17", *CFLAGS, "-c", str(src), "-o", str(obj)], check=True)
        objs.append(obj)
    if not objs:
        sys.exit("no sources")
    exports = []
    rows = []
    for obj in objs:
        nm = subprocess.run(["llvm-nm", "--defined-only", "--extern-only", str(obj)],
                            check=True, capture_output=True, text=True).stdout
        for line in nm.splitlines():
            parts = line.split()
            if len(parts) == 3 and parts[1] in ("T", "t"):
                exports.append(parts[2])
                rows.append(f"{parts[2]}\t{obj.relative_to(BUILD / 'obj').with_suffix('.cpp')}")
    # MSVC-runtime stub the compiler references whenever floats are used; no CRT is linked.
    stub = BUILD / "fltused.c"
    stub.write_text("int _fltused = 0;\n")
    subprocess.run(["clang", *CFLAGS, "-c", str(stub), "-o", str(BUILD / "fltused.obj")], check=True)
    objs.append(BUILD / "fltused.obj")
    dll = BUILD / "recon.dll"
    subprocess.run(["lld-link", "/dll", "/noentry", "/nodefaultlib", "/base:0x10000000", "/fixed",
                    f"/out:{dll}", *[f"/export:{e[1:] if e.startswith('_') else e}" for e in exports], *map(str, objs)], check=True)
    (BUILD / "recon.symbols.tsv").write_text("symbol\tsource\n" + "\n".join(rows) + "\n")
    print(f"{dll}: {len(exports)} functions from {len(objs)} files")


if __name__ == "__main__":
    main()
