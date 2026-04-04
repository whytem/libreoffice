# Computational Substrate Storage And Wiring Plan

Status: implementation-ready planning doc

## Purpose

This document defines the most actionable path from the current bounded
computational-substrate result toward the much larger target where
`spreadsheet_engine/` would own:

- computation-facing cell storage
- listener registration policy
- broadcaster indexing and wiring
- the live dependency graph updates that currently still terminate in Calc

It is intentionally not a direct "move `ScDocument` into the engine" plan.

The recommended path is narrower and more practical:

- first make the engine authoritative for a mutable sidecar substrate and
  graph deltas on the already-admitted slice
- then let Calc act as the temporary host that applies those engine-issued
  deltas into the retained live containers
- only after that consider physical storage and container migration

## Why This Is The Best Next Path

The current project has already proven:

- engine-owned computational and graph shadows
- engine-owned execution-facing IR
- bounded authority and bounded rollout on a narrow structural slice
- exact verification and rollback infrastructure

What it has not proven is:

- broad storage replacement
- broad listener/broadcaster ownership transfer
- broad token-container ownership transfer

The shortest path from the current state to those larger goals is therefore:

1. replace rebuild-only shadowing with engine-maintained mutable state
2. make the engine authoritative for graph and wiring decisions before it owns
   the physical containers
3. move physical storage only after the engine already drives the live shape

## Plan Goal

Establish whether `spreadsheet_engine` can become the real authority for cell,
listener, and broadcaster state on the already-admitted narrow slice by using
an engine-maintained mutable substrate plus engine-issued graph deltas, while
Calc temporarily remains the host that realizes those deltas.

The goal is not broad document migration. The goal is to create a clean,
exactly-verified handoff path for the admitted slice that could later support:

- broader storage ownership
- broader listener/broadcaster ownership
- and ultimately a cleaner computational document boundary

## Entry Boundary

This plan begins from the currently settled state:

- the first-stage extraction boundary is complete and successful
- the computational-substrate architecture program closed with a narrow
  proceed result
- the opt-in narrow rollout exists and is stable on the admitted scalar and
  structural slice
- Calc still owns:
  - document storage and mutation APIs
  - formula-cell object lifetime
  - listener and broadcaster container storage
  - final live verification and rollback

This plan must therefore treat mutable substrate and wiring authority as a new
proof surface, not as an already-admitted extension of the current rollout.

## Non-Goals

This plan should not attempt to:

- move UI, UNO, rendering, import/export, or persistence into the engine
- replace all `ScDocument` storage in one step
- migrate `ScTokenArray` ownership as part of the first implementation sweep
- widen immediately into shared-group, named-range-sensitive, sheet-level, or
  undo-like structural behavior
- remove exact verification or rollback from the admitted slice
- claim broad listener/broadcaster container migration before the authority
  path is proven

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note for the mutable sidecar substrate and graph
   delta authority surface
2. engine-maintained mutable computational state for the admitted slice
3. engine-generated listener, broadcaster, formula-tree, formula-track, and
   recalc graph deltas for the admitted slice
4. Calc-side apply adapters that realize those deltas into the retained live
   host containers
5. checked-in differential evidence showing exact or explicitly-classified
   results when the engine drives that live path
6. a checked-in decision record stating whether:
   - the engine can become the authority for mutable sidecar state and graph
     deltas on the admitted slice
   - the path should remain validation-only
   - or the larger storage/wiring migration should be deferred again

## Workstreams

### 1. Freeze The Mutable Substrate And Graph-Delta Contract

Define the exact authority surface before implementation begins.

This contract should freeze:

- which workbook and mutation classes are in scope:
  - the already-admitted scalar lifecycle slice
  - the already-admitted single-sheet structural slice
  - ordinary scalar formulas only
  - clean baseline only
- which mutable state must become engine-authored:
  - cell population on the admitted slice
  - formula-tree membership
  - formula-track membership
  - normalized broadcaster/listener projection
- which live outputs the engine must produce:
  - listener add/remove deltas
  - broadcaster add/remove deltas
  - formula-tree / formula-track deltas
  - recalc queue deltas already covered by the current pilot
- which classes remain out of contract:
  - shared groups
  - named-range-sensitive structural behavior
  - sheet-level structural edits
  - clipboard, load-time, move, and undo-like flows

Required artifact:

- [COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_CONTRACT.md)

### 2. Build Engine-Maintained Mutable Substrate State

Replace rebuild-only shadow behavior with engine-maintained mutable state on
the admitted slice.

This workstream should:

- introduce an engine-owned mutable computational-state container for the
  admitted slice
- apply admitted edits incrementally instead of rebuilding from Calc after
  every mutation
- preserve the current shadow builders as validation or bootstrap helpers
- keep the mutable state value-semantic and free of hidden Calc pointer
  ownership

Required artifact:

- [COMPUTATIONAL_SUBSTRATE_MUTABLE_SUBSTRATE_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_MUTABLE_SUBSTRATE_IMPLEMENTATION.md)

### 3. Build Engine-Owned Graph And Wiring Deltas

Make the engine authoritative for the live dependency-side decisions on the
admitted slice before moving the physical containers.

This workstream should:

- derive listener add/remove actions from the engine-maintained substrate
- derive broadcaster add/remove actions from the engine-maintained substrate
- derive formula-tree and formula-track delta actions
- preserve exact queue and graph expectations
- classify any retained opaque Calc repair as proof failure or defer

Required artifact:

- [COMPUTATIONAL_SUBSTRATE_GRAPH_DELTA_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_GRAPH_DELTA_IMPLEMENTATION.md)

### 4. Build Calc Apply Adapters For Engine-Issued Deltas

Teach Calc to realize the engine-owned deltas into the retained live host
containers.

This workstream should:

- add narrow compat adapters around the current host surfaces:
  - `StartListeningArea`
  - `EndListeningArea`
  - `StartListeningCell`
  - `EndListeningCell`
  - formula-tree and formula-track insertion/removal
- keep Calc as the temporary owner of:
  - live container residency
  - final rollback
  - document mutation APIs
- ensure the engine, not Calc-local ad hoc logic, is the thing deciding the
  admitted wiring outcome

Required artifact:

- [COMPUTATIONAL_SUBSTRATE_WIRING_APPLY_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_WIRING_APPLY_IMPLEMENTATION.md)

### 5. Freeze Differential Authority Evidence

Run the bounded proof cycle and record the results.

This evidence note should summarize:

- exact same-mutation comparisons between:
  - engine-maintained mutable substrate
  - engine-generated graph deltas
  - Calc live after-state
- rollback-triggering cases
- repair-detected cases
- memory and performance observations
- whether the engine-driven path is actually cleaner than the existing
  rebuild-and-verify model

Required artifact:

- one checked-in differential authority evidence note

### 6. Freeze The Storage And Wiring Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-authoritative mutable substrate and graph deltas on the
  admitted slice
- keep the path validation-only
- or defer the larger storage/wiring migration again

The decision record must also say what the next adjacent concern becomes if
this path succeeds:

- physical cell-storage residency migration
- live broadcaster/listener container migration
- or token-container authority reassessment

Required artifact:

- one checked-in storage-and-wiring decision record

## Target Surfaces

The first implementation sweep should expect to touch:

- engine substrate types under
  `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- compat builders and apply adapters under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- Calc host surfaces including:
  - [document.hxx](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx)
  - [listenercontext.hxx](/home/ubuntu/repos/libreoffice/sc/inc/listenercontext.hxx)
  - [mtvelements.hxx](/home/ubuntu/repos/libreoffice/sc/inc/mtvelements.hxx)
  - [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
  - [document10.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/document10.cxx)
- standalone proof lanes under:
  - [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
  - [computational_graph_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_graph_tests.cxx)
  - [computational_execution_ir_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_execution_ir_tests.cxx)
- Calc differential coverage under:
  - [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  - [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
  - [ucalc_compile_diff.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_compile_diff.cxx)
- top-level docs:
  - [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
  - [README.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/README.md)

## Recommended Execution Order

Run this plan in the following order:

1. freeze the mutable substrate and graph-delta contract
2. build engine-maintained mutable substrate state
3. build engine-owned graph and wiring deltas
4. build Calc apply adapters for engine-issued deltas
5. freeze differential authority evidence
6. freeze the storage and wiring decision

## Validation Contract

The minimum closeout contract should be:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_ir_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

If the mutable substrate or graph-delta path introduces new verdict
categories, those categories must be covered in checked-in Calc and standalone
tests before the plan can close.

## Exit Criteria

This plan is complete only when:

- the mutable sidecar substrate and graph-delta authority boundary is frozen
- the engine can maintain admitted computational state incrementally
- the engine can generate wiring deltas for the admitted slice
- Calc can realize those deltas through explicit adapters
- exact differential evidence shows whether the engine is now the real
  authority for that admitted live path
- the final decision says whether this path should become the next concrete
  bridge toward physical storage and listener/broadcaster migration

If the proof does not support moving forward, the plan still closes
successfully when it leaves behind:

- a bounded validation-only mutable substrate path
- explicit reasons the live wiring authority handoff failed
- and a narrower next adjacent concern
