# Retail builds

The goal is the final retail build. The 2010/2011 dedicated-server builds in `target.toml` are the
**reference**: their private PDB and MAP give names, types, source files, lines and struct layouts. The
retail build has none of that and has to inherit it function by function.

## The builds

| Build | Exe (PDB name in debug dir) | Linked | Config | sha256 |
|---|---|---|---|---|
| CoDMPServer | CoDMPServer.exe | 2010-11-05 | `CoDMPRelease DedicatedServer - PC` | 4e325afa…eff5 |
| CoDMPServer_s | CoDMPServer_s.exe | 2011-08-31 | (server, partial PDB) | 3eb933d8…c9e5 |
| **t5mp** (retail MP) | BlackOpsMP.exe (`CoDMPSteam_CEG_bin\BlackOpsMP.pdb`) | 2011-11-03 | `CoDMPSteam_CEG` | 6a544494…c3fb |
| t5sp (retail SP) | BlackOps.exe (`CoDSteam_CEG_bin\BlackOps.pdb`) | 2011-09-01 | `CoDSteam_CEG` | f968ce5b…230a |

No PDB, MAP or Rich header ships with the retail exes. Multiplayer (`t5mp`) comes first.

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
