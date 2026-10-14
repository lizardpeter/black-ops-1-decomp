# Verification

How a function earns `compiles` and `matches` in `records/functions.tsv`.

## compiles

`tools/build_recon.py` builds everything under `src/` with clang targeting `i686-pc-windows-msvc`
(MSVC calling convention and C++ name mangling) into `build/recon.dll`, exporting every function under
its decorated name.

Because the tests look each function up by the **original decorated name from the linker MAP**, a
reconstruction with the wrong parameter types, return type, constness or `extern "C"`-ness isn't found,
and fails.

## matches (current method: differential)

`tests/differential/run.py` maps the original `CoDMPServer.exe` (sha256-checked) and `recon.dll` into one
Unicorn x86-32 emulator. For each test case it calls the original address and the reconstruction with
identical cdecl arguments, then compares:

- the return value (EAX for integers, x87 ST0 for floats, bit-exact; NaN equals NaN),
- every buffer passed by pointer, byte for byte.

Inputs mix random values with edge cases (±0, NaN, ±inf, integer limits). The seed is fixed, so runs
are reproducible.

`--check-records` fails CI if `records/` claims `matches` for a function without a passing test.

### Limits

- This is behavioural equivalence on the tested inputs. It is not byte equivalence: `if (i > 127)` and
  `if (i >= 127)` behave identically in `ClampChar` and both pass.
- Functions that call other functions run the original callees inside the original image. A reconstruction
  that calls its own reconstructed callees is compared as a whole.
- Functions that touch global game state need that state set up in the test first.

## Byte matching (planned)

The originals were built by MSVC 2008 (VC90) with optimisation effectively off: frame pointers everywhere,
every value reloaded from its stack slot. That makes exact byte matching realistic. It needs the VC90 `cl.exe`
on a Windows runner and an object-level diff against `analysis/<build>/ghidra/functions.tsv` (`bytes_sha256`,
relocations masked). Once that runs, `matches` will require it, and the differential test stays as a
second check.
