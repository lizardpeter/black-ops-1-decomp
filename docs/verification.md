# Verification

How a function earns `compiles` and `matches` in `records/functions.tsv`.

## compiles

`tools/build_recon.py` builds everything under `src/` with clang targeting `i686-pc-windows-msvc`
(MSVC calling convention and C++ name mangling) into `build/recon.dll`, exporting every function under
its decorated name.

Because the tests look each function up by the **original decorated name from the linker MAP**, a
reconstruction with the wrong parameter types, return type, constness or `extern "C"`-ness isn't found,
and fails.

## Hybrid link

The reconstruction is linked against the original binary. Any function or global it uses but does not
define yet resolves to the original's address from the MAP (`build/hybrid.tsv` lists them): functions
through a `push <va>; ret` thunk into the original code, globals as absolute symbols at the original
address. So a function can be reconstructed and tested before anything it calls is, and reconstructed
code reads and writes the same globals the original does.

`tools/recon/` is the build environment, not game source: VC90-style headers (`math.h`, where `atan2f` is
an inline `(float)atan2(...)` exactly as in VC90) and shims that route `atan2`, `sin`, `pow`, ... to the
original CRT's `___libm_sse2_*` routines, which is what VC90 emits with SSE2 enabled. Both sides therefore
run the very same math library.

## matches (current method: differential)

`tests/differential/run.py` maps the original `CoDMPServer.exe` (sha256-checked) and `recon.dll` into one
Unicorn x86-32 emulator. For each test case it calls the original address and the reconstruction with
identical cdecl arguments, then compares:

- the return value (EAX for integers, x87 ST0 for floats, bit-exact; NaN equals NaN),
- every buffer passed by pointer, byte for byte.

Inputs come from hand-written `CASES` in `tests/differential/test_*.py` when a function needs structured
input, and are otherwise generated from the original prototype (`auto.py` demangles the decorated name).
They mix random values with edge cases (±0, NaN, ±inf, integer limits); seeds are fixed per function.

Every write a call makes into the original image (its globals) is journaled, compared between original
and reconstruction, and rolled back, so calls never see each other's state. Inputs on which the original
itself crashes are skipped; a function with too few usable inputs is reported untestable, never passed.

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
