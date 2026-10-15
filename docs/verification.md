# Verification

How a function earns `compiles` and `matches` in `records/functions.tsv`.

## compiles

`tools/build_recon.py` builds everything under `src/` with clang targeting `i686-pc-windows-msvc`
(MSVC calling convention and C++ name mangling) into `build/recon.dll`, exporting every function under
its decorated name.

Because the tests look each function up by the **original decorated name from the linker MAP**, a
reconstruction with the wrong parameter types, return type, constness or `extern "C"`-ness would not be
found. The build therefore stops with "prototype differs from the original's decorated name" when a
defined symbol isn't an original decorated name but its plain name is an original function (both names are
printed). Helpers that share a name on purpose (VC90's `math.h` float overloads) are listed, with the
reason, in `PROTOTYPE_ALLOWLIST` in the build script.

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
- every buffer passed by pointer, byte for byte,
- every byte written into the original's globals, the thread's TLS block, the harness's dvars and the
  test's fixture memory,
- every Win32 call made, in order, with its arguments (pointers to the callers' own locals are compared
  through what was written to them, not by address), and the final state of the harness's window model.

Inputs come from hand-written `CASES` in `tests/differential/test_*.py` when a function needs structured
input, and are otherwise generated from the original prototype (`auto.py` demangles the decorated name).
They mix random values with edge cases (±0, NaN, ±inf, integer limits); seeds are fixed per function.

Each call starts from Windows' thread FPU state (x87 control word 0x27F, MXCSR 0x1F80, all floating-point
exceptions masked), as the original ran with.

Every write a call makes into the original image (its globals) is journaled, compared between original
and reconstruction, and rolled back, so calls never see each other's state. Inputs on which the original
itself crashes, or doesn't return within the instruction limit, are skipped; a function with too few usable
inputs is reported untestable, never passed.

### The emulated machine

Code under test reaches far beyond its own file, so the harness gives it enough of a Windows process to run
the original callees:

- **Win32.** The original's import table points at stubs backed by a small model: one window (rectangle,
  client-area offset), the cursor, the foreground window, the `ShowCursor` counter, the time
  (`timeGetTime`), and a gamepad on port 0 (`XInputGetState`/`GetCapabilities`/`SetState`, imported by
  ordinal from xinput1_3). Critical sections, `GetLastError`, `HeapAlloc`, `OutputDebugStringA` and the few
  other calls the CRT makes are stubbed too. Every other import is a trap that stops the run naming the
  import, so a missing stub shows up as "unstubbed import X" rather than a jump into random code. The
  reconstruction reaches the same stubs: its `__imp__` references resolve to the original's import slots.
- **One thread.** FS selects a TEB (through a small GDT; Unicorn ignores `FS_BASE` in 32-bit mode) whose
  TLS array points at a fresh copy of the exe's `__declspec(thread)` block. The engine's per-thread values
  (`Sys_GetValue`) point at zeroed buffers.
- **The CRT's per-thread data.** At startup the harness runs the original `__mtinitlocks` and `__initptd`
  (with the CRT heap globals set as `_heap_init` would), keeps the resulting `_tiddata` as a template, and
  hands it out through `TlsGetValue`/`FlsGetValue`. `rand()`, `_getptd` and friends then work, which is what
  made `random`, `crandom` and `GaussianRandom` testable.
- **Dvars.** Every `const dvar_s *` global in the MAP, and every file-static one in the PDB, that is NULL
  points at its own default dvar (its name, type bool, value 0), as if every dvar were registered. A test's
  `setup(fixture, state)` builds the dvars it cares about with real values (`fixtures.make_dvar`).
- **Fixtures.** `setup` can build structs in fixture memory (the client's `clientActive_t`, a weapon
  definition) and point the original's globals at them. Writes into fixture memory are compared like globals.

What it does not provide yet: engine subsystems that need their init to have run (the script string table
used by dvar registration, the memory tree), real rendering, files and the network. Functions that need
those stay at `compiles` with a note saying which.

### Checking the checks

When a harness feature is added, deliberate bugs must fail: an off-by-one in `IN_ClampMouseMove` and a wrong
target in `IN_ShowSystemCursor` (Win32 call log and window model); truncation instead of the biased x87
rounding, `I_fmin` with swapped arguments, and the speed cap computed in float instead of double in
`CL_MouseMove`. A rewrite that is provably equivalent (the `rate` divide in float instead of double, which
double rounding cannot distinguish) passes, as it should. Inputs are added until each deliberate bug fails:
`CL_MouseMove` has targeted inputs where the speed cap decides the result, and out-of-domain values (NaN,
infinity) that the game's dvar limits keep out but the code still has a defined answer for.

`--check-records` fails CI if `records/` claims `matches` for a function without a passing test.

### Limits

- This is behavioural equivalence on the tested inputs. It is not byte equivalence: `if (i > 127)` and
  `if (i >= 127)` behave identically in `ClampChar` and both pass.
- Functions that call other functions run the original callees inside the original image. A reconstruction
  that calls its own reconstructed callees is compared as a whole.
- Functions that touch global game state need that state set up in the test first (see "The emulated
  machine").
- A value the original leaves uninitialized (stack garbage) differs between any two builds by construction;
  tests set up states where it is defined and the case is recorded in `records/bugs.tsv` (B0004).

## Byte matching (planned)

The originals were built by MSVC 2008 (VC90) with optimisation effectively off: frame pointers everywhere,
every value reloaded from its stack slot. That makes exact byte matching realistic. It needs the VC90 `cl.exe`
on a Windows runner and an object-level diff against `analysis/<build>/ghidra/functions.tsv` (`bytes_sha256`,
relocations masked). Once that runs, `matches` will require it, and the differential test stays as a
second check.
