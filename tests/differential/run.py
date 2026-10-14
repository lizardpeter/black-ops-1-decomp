#!/usr/bin/env python3
"""Run every differential test and check records/functions.tsv against the results.

    python3 tests/differential/run.py [--check-records]

Writes build/test-results.tsv (build, va, symbol, cases, result). With --check-records, fails if
records/functions.tsv claims `matches` (or better) for a function without a passing test here.
"""
import csv
import importlib.util
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from harness import ROOT, Image, Machine, original_exe  # noqa: E402

BUILD = "CoDMPServer"
MATCHED = {"matches", "integrated"}


def same(a, b):
    if isinstance(a, float) and isinstance(b, float):
        if math.isnan(a) and math.isnan(b):
            return True
        return struct.pack("<d", a) == struct.pack("<d", b)
    return a == b


def export_name(symbol):
    # extern "C" x86 names carry a leading underscore in the MAP but not in the DLL's export table
    return symbol[1:] if symbol.startswith("_") else symbol


def main():
    recon = Image(ROOT / "build" / "recon.dll")
    m = Machine(Image(original_exe(BUILD)), recon)
    results, failed = [], 0
    for test in sorted(Path(__file__).parent.glob("test_*.py")):
        spec = importlib.util.spec_from_file_location(test.stem, test)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        for case in mod.CASES:
            va, sym = case["va"], case["symbol"]
            target = recon.exports.get(export_name(sym))
            if target is None:
                verdict = "missing: no export with the original decorated name"
            else:
                verdict = "pass"
                for args in case["inputs"]:
                    want = m.call(va, args, case["ret"])
                    got = m.call(target, args, case["ret"])
                    if not (same(want[0], got[0]) and want[1] == got[1]):
                        verdict = f"FAIL args={args!r} original={want!r} recon={got!r}"
                        break
            failed += verdict != "pass"
            results.append([BUILD, f"{va:08X}", sym, len(case["inputs"]), verdict])
            print(f"{'ok  ' if verdict == 'pass' else 'FAIL'} {va:08X} {sym}  ({len(case['inputs'])} cases)"
                  + ("" if verdict == "pass" else f"\n     {verdict[:400]}"))
    out = ROOT / "build" / "test-results.tsv"
    out.write_text("build\tva\tsymbol\tcases\tresult\n" + "".join("\t".join(map(str, r)) + "\n" for r in results))

    if "--check-records" in sys.argv:
        passed = {(r[0], r[1]) for r in results if r[4] == "pass"}
        with open(ROOT / "records" / "functions.tsv", newline="") as f:
            for r in csv.DictReader(f, delimiter="\t"):
                if r["status"] in MATCHED and (r["build"], r["va"]) not in passed:
                    print(f"FAIL records claim {r['status']} for {r['build']}:{r['va']} without a passing test")
                    failed += 1
    print(f"{len(results) - failed}/{len(results)} passed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
