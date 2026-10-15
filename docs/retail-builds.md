# Retail builds

The goal is the final retail build. The 2010/2011 dedicated-server builds in `target.toml` are the
**reference**: their private PDB and MAP give names, types, source files, lines and struct layouts. The
retail build has none of that and has to inherit it function by function.

## The builds

| Build | Exe (PDB name in debug dir) | Linked | Config | sha256 |
|---|---|---|---|---|
| CoDMPServer | CoDMPServer.exe | 2010-11-05 | `CoDMPRelease DedicatedServer - PC` | 4e325afa…eff5 |
| CoDMPServer_s | CoDMPServer_s.exe | 2011-08-31 | (server, partial PDB) | 3eb933d8…c7b9 |
| **t5mp** (retail MP) | BlackOpsMP.exe (`CoDMPSteam_CEG_bin\BlackOpsMP.pdb`) | 2011-11-03 | `CoDMPSteam_CEG` | 6a544494…c3fb |
| t5sp (retail SP) | BlackOps.exe (`CoDSteam_CEG_bin\BlackOps.pdb`) | 2011-09-01 | `CoDSteam_CEG` | f968ce5b…230a |

No PDB, MAP or Rich header ships with the retail exes. Multiplayer (`t5mp`) comes first.

### Provenance

The retail exes come from Plutonium's executable set (R2 `Downloads/plu_exes/`, alongside its t4/t6/iw5/iw6
exes), named `t5mp.exe` / `t5sp.exe` rather than the retail `BlackOpsMP.exe` / `BlackOps.exe`.

- Both carry Valve Corporation Authenticode signatures (as Steam CEG builds did) and their original 2011
  link timestamps and Treyarch build paths.
- Neither signature verifies: the recomputed Authenticode digest differs from the signed one, and the PE
  checksum doesn't match either. The files were modified after Valve signed them, most likely to remove
  the CEG DRM.

So almost all of the code is Treyarch's 2011 retail output, but some functions (DRM checks) are likely
patched. Without an unmodified Steam copy to diff against, patched functions have to be found from the
code itself; once identified they are recorded as patched, not as original.

## What the retail build looks like

Measured with `tools/`-style throwaway scripts against CoDMPServer (numbers for t5mp):

- **Code is readable.** `.text` entropy is normal for x86 (6.73). CEG has not encrypted the code.
- **Optimized, frame pointers omitted.** 2,780 `push ebp; mov ebp, esp` prologues against 25,176 in the
  server build (44 vs 288 per 100 KB of code), and 6.3 MB of `.text` against 8.7 MB even though the client
  also carries the renderer. Byte-matching retail code against the server build is therefore not possible;
  the reconstruction is shared, but retail functions are compiled differently.
- **Asserts compiled out.** 220 `codsrc` path strings survive (1,001 in the server build); `com_math.cpp`'s
  is gone.
- **Whole-program optimization.** Functions of one source file are spread over ~153x the address span they
  occupy in the server build (median over 245 files), and only 62% of anchored functions keep their
  within-file order. This is consistent with link-time code generation (/GL /LTCG) plus an optimized
  function order: functions are reordered and inlined across files. Position in the binary cannot be used
  for matching, and many small server functions will not exist on their own in retail.

## Matching plan

Each retail function gets the identity of a server function only with recorded evidence and a confidence:

1. **Unique strings.** 3,075 server functions reference a string no other server function references, and
   that string is referenced exactly once in t5mp: 11% of the server's 28,050 functions anchor directly.
2. **Call-graph propagation** from anchors: a callee or caller whose position is unique among the matched
   neighbours inherits the match, iterated.
3. **Structural similarity** for the rest: referenced constants and globals, callee sets, basic-block shape.
4. **Inlined functions** are recorded as inlined into the retail function that contains them, not as matches.

The reconstructed source is then tested against retail with the same differential harness, at the matched
addresses.

## Matcher validation

`tools/match/match.py` is checked on a pair with ground truth: CoDMPServer (reference) against
CoDMPServer_s matched blind, then scored with `tools/match/validate.py` against _s's own MAP names.

| Method | Matches | Precision |
|---|---:|---:|
| unique strings | 2,741 | 98.0% |
| unique constants | 13 | 100% |
| callee propagation | 1,658 | 97.0% |
| caller propagation | 1,516 | 97.4% |
| **all** | **5,928 (32.6% of _s)** | **97.6%** |

Some "wrong" pairs are the same function with a changed prototype (e.g. `BG_LoadAnim` gained a
parameter), so precision is slightly understated. Both server builds are compiled the same way; retail
is harder, so its match rate and precision will be lower and are reported separately.

## t5mp results

`match.py CoDMPServer t5mp` (output: `analysis/t5mp/match/functions.tsv`). Ghidra finds 19,121 functions
in t5mp, 18,959 of them non-thunk.

| Method | Matches |
|---|---:|
| unique strings | 2,373 |
| unique constants | 16 |
| callee propagation | 2,651 |
| caller propagation | 1,888 |
| **all** | **6,928 (36.5% of t5mp)** |

There is no ground truth for retail, so precision is estimated two ways:

- **Call-graph consistency.** For each matched pair, how many of the retail function's matched callees
  map to callees of the server function: 88.2% for string matches, 88.6% callee, 90.7% caller, 97.6%
  constant. Propagated matches are as consistent as the string matches. The rest is mostly retail
  inlining, which removes or adds call edges.
- **Manual review** of 8 randomly sampled propagated matches against the server pseudocode
  (`Huff_Init`, `EmitOpcode`, `LiveContracts_GetContractName`, `CG_UpdateViewOffset`,
  `Bot_UpdateMovement`, `path_constraint_create`, `CXAPOBase::ValidateFormatDefault`, a libwww request
  builder): all 8 correct.

Matches by top-level source folder: third-party libs 1,409, other external code 505, universal 450,
live 376, ui 349, gfx_d3d 336, qcommon 319, bgame 271, cgame 258, cgame_mp 253, game_mp 238.
