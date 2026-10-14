# black-ops-1-decomp

Decompilation of **Call of Duty: Black Ops (T5)**, PC multiplayer dedicated server, starting from the
original executables with their private PDBs and linker MAPs.

| Build | Date | PDB coverage |
|---|---|---|
| `CoDMPServer` (primary) | 2010-11-05 | 24.6k functions with types + line tables, 1,939 compilands |
| `CoDMPServer_s` | 2011-08-31 | types + publics, 3.1k private procs |

Compiler: MSVC 2008 (VC90), x86. Original source root: `codsrc/src/`.

- How the repo is organised and the rules for changing it: [AGENTS.md](AGENTS.md)
- Progress: [STATUS.md](STATUS.md)
- What is being reversed and where the originals live: [target.toml](target.toml)

## Pipeline

1. `tools/pdb/extract.py` turns each build's PDB + MAP into `analysis/<build>/pdb/` and `map/`.
2. `.github/workflows/ghidra.yml` imports each exe with its PDB into Ghidra, exports functions, call graph,
   symbols, strings and types, decompiles every function, and commits the results to
   `analysis/<build>/` grouped by original source file.
3. `tools/status.py` combines `analysis/` with the authored `records/` into `STATUS.md` and the graph.

The game files themselves are never committed.
