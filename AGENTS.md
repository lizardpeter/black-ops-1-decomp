# Working in black-ops-1-decomp

Decompilation of Call of Duty: Black Ops (T5, PC). Read this before changing anything.

## The one rule: generated vs authored

Every file is exactly one of these. They never share a folder.

| Kind | Where | Who writes it |
|---|---|---|
| **Generated** | `analysis/<build>/` | Only the scripts in `tools/`. Never hand-edit. Re-running the pipeline on the same input reproduces it. |
| **Authored** | `src/`, `include/`, `tests/`, `docs/`, `records/` | People and agents. |

If a generated file is wrong, fix the script that wrote it and re-run. Don't patch the output.

## Layout

```
target.toml          builds (sha256 + R2 key of each original file), project id
analysis/<build>/    GENERATED per build
  functions.tsv        every function: address, size, name, signature, module, bytes hash
  symbols.tsv          non-function symbols (globals, labels, strings)
  callgraph.tsv        caller -> callee edges
  types.h              every PDB/Ghidra data type as C
  pseudocode/<module>.c   Ghidra decompiler output, grouped by original compiland
  disasm/<module>.asm     disassembly, same grouping
  pdb/                 llvm-pdbutil dumps (modules, publics, sources)
  map/                 parsed linker MAP
  ghidra/              Ghidra exports + the saved Ghidra project (project.tar.gz, split into parts)
  logs/                pipeline logs
records/             AUTHORED small facts; the only input to the graph
  functions.tsv        status of every reconstructed function (see the ladder below)
  bugs.tsv             everything in the original worth fixing (see "Faithful and enhanced")
src/ include/        AUTHORED faithful reconstruction, mirroring the original source tree from the PDB
tests/               match / differential tests
docs/<topic>.md      subsystems, formats, protocols. Topic names, never dates.
tools/               pipeline scripts (ghidra/, pdb/, map/, graph/)
```

Folder and file names describe *what*, never *when*. Dates belong in commit messages.

## Faithful and enhanced

`src/` is the **faithful** decomp: original behaviour, bugs included, verified function by function. Never fix
a bug in `src/`. An **enhanced** build (the faithful code plus overrides in `enhanced/`, each recorded in
`records/patches.tsv`) is created with the first enhancement.

**Everything worth fixing goes in `records/bugs.tsv`**, not only outright bugs: id (`B` + 4 digits), build, va,
function, source, kind (`security`, `crash`, `hang`, `logic`, `ub`, `robustness`, `limit`, `precision`,
`performance`, `quality`), priority (`high`, `medium`, `low`), description, evidence (the test or disassembly
that shows it, and whether a current caller can trigger it), and `enhanced` (`planned`, `fixed <patch id>`, or
`keep` when the original behaviour is wanted). It is the enhanced build's fix queue.

## Function status ladder

Every function in `records/functions.tsv` has exactly one status:

`located -> named -> typed -> decompiled -> compiles -> matches -> integrated`

- `located`: entry and size known (Ghidra)
- `named`: original name known (PDB/MAP)
- `typed`: original prototype known (PDB)
- `decompiled`: authored source exists in `src/`
- `compiles`: that source builds in CI
- `matches`: compiled code matches the original bytes (or passes the differential test)
- `integrated`: linked into the runnable reconstruction

`STATUS.md` and the graph are both computed from this file. Don't track progress anywhere else.

## Graph

Graph `re_t5` on the Graph DB is **derived**. `tools/graph/sync.py` turns `records/` plus
`analysis/<build>/functions.tsv` into idempotent Cypher. Never write ad-hoc Cypher to the graph;
change records and re-sync. The graph holds identity, relationships and status only, with no code text.

## Originals

The game files (exe, pdb, map) are never committed. They live in R2 under the keys in
`target.toml`, and the pipeline verifies their sha256 before use.
