# Computational Substrate Mutation Entry Plan

Status: implementation-ready plan

## Purpose

This document defines the next bounded migration step after the completed
[COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_DECISION_RECORD.md).

That decision closed with the strongest admitted-slice boundary proven so far:

- the engine owns admitted-slice resident cell storage
- the engine owns admitted-slice resident wiring containers
- the engine owns admitted-slice formula-cell lifetime decisions
- the engine owns mutable computational state plus graph, wiring, and queue
  decisions on that slice
- Calc can realize admitted live cells, wiring, and formula-cell objects from
  that engine-owned state with exact verification
- Calc still owns mutation entry, live object realization, and rollback

The next adjacent concern is therefore no longer resident storage, resident
wiring, or formula-cell lifetime. It is direct admitted-slice mutation entry.

This plan is not a broad document-host transplant. It is the first
implementation plan for making the engine the entry authority for admitted
mutations on the admitted slice while Calc temporarily remains the
apply/realize/verify/rollback host.

## Why This Is The Best Next Path

The current project has already proven:

- engine-owned admitted resident cell storage
- engine-owned admitted resident wiring containers
- engine-owned admitted formula-cell lifetime
- engine-owned mutable computational state
- engine-owned graph, wiring, and recalc decisions
- exact Calc-side realization of admitted live cells, wiring, and formula
  objects from engine-owned state
- exact queue, computational, and graph verification plus explicit rollback

What it has not yet proven is:

- direct engine-owned mutation entry on the admitted slice
- broad mutation-entry migration outside the admitted slice
- broad document-host mutation authority transfer

The shortest path from the current boundary to broader engine-owned live
authority is therefore:

1. make admitted mutation requests enter through the engine first
2. keep Calc as the temporary apply, realization, and rollback host
3. preserve engine-owned admitted resident state and after-state decisions
4. consider broader mutation-authority transfer only after admitted mutation
   entry is proven stable

## Plan Goal

Determine whether `spreadsheet_engine` can safely own the entry boundary for
admitted scalar lifecycle and admitted narrow structural mutations while Calc
applies the resulting engine-issued deltas, realizes live objects, and
continues to perform final verification and rollback.

The goal is to decide one explicit question:

- proceed with engine-owned admitted-slice mutation entry
- keep mutation entry validation-only
- or defer mutation entry again because the entry-to-realization model is not
  yet stable enough

## Entry Boundary

This plan begins from the currently settled formula-cell-lifetime boundary:

- the first-stage extraction boundary is complete and stable
- the computational-substrate program closed with a narrow authority result
- the opt-in narrow rollout is complete and stable on the admitted scalar and
  structural slice
- the completed cell-storage residency proof cycle established:
  - engine-owned admitted resident cell storage
  - exact Calc-side cell mirroring from engine-owned state
- the completed wiring-container residency proof cycle established:
  - engine-owned admitted resident wiring containers
  - exact Calc-side live wiring realization from engine-owned state
- the completed formula-cell-lifetime proof cycle established:
  - engine-owned admitted formula-cell lifetime decisions
  - exact Calc-side realization of admitted live formula-cell objects from
    engine-owned state
- Calc still owns:
  - `ScDocument` mutation entry APIs
  - live object realization
  - final rollback

This plan must therefore treat engine-owned mutation entry as a new proof
surface, not as an already-admitted extension of the current rollout.

## Non-Goals

This plan should not attempt to:

- move UI, UNO, rendering, import/export, or persistence into the engine
- replace all `ScDocument` mutation APIs in one sweep
- migrate rollback out of Calc in the same cycle
- migrate `ScTokenArray` ownership as part of this first mutation-entry pass
- widen directly into shared-group-sensitive, named-range-sensitive,
  off-sheet, or sheet-wide structural behavior
- weaken exact queue, computational, graph, or rollback requirements
- treat live object realization as already migrated out of Calc

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted mutation-entry surface
2. a checked-in schema note for engine-owned mutation request, translation,
   and verdict records
3. an implementation note for the engine-owned admitted-slice mutation-entry
   path
4. an implementation note for Calc apply/realization of engine-issued
   mutation deltas
5. a checked-in evidence note covering exact differential behavior and
   rollback-triggering cases
6. a checked-in decision record saying whether admitted-slice mutation entry:
   - proceeds
   - remains validation-only
   - or is deferred again

## Workstreams

### 1. Freeze The Mutation-Entry Contract

Freeze the exact admitted mutation-entry surface before any implementation
work begins.

The contract should name:

- the admitted workbook and mutation classes
- the exact mutation-entry operations that may become engine-owned
- retained Calc-owned surfaces that stay host-only in this cycle
- apply, realization, verification, and rollback requirements
- what counts as exact success versus immediate defer

Required artifact:

- one checked-in mutation-entry contract note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_CONTRACT.md)

### 2. Define The Engine-Owned Mutation-Entry Schema

Define the engine-owned entry shape that will act as the source of truth for
admitted mutation requests on the admitted slice.

This schema note should define:

- engine-owned admitted mutation request records
- stable mutation identity and normalized payload rules
- create, replace, remove, and admitted structural entry semantics
- exact versus normalized-equivalent after-state comparisons
- forbidden Calc-local mutation-entry shortcuts

Required artifact:

- one checked-in engine-owned mutation-entry schema note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_SCHEMA.md)

### 3. Build The Engine-Owned Admitted-Slice Mutation-Entry Path

Extend the current admitted resident-state and lifetime path so the engine
owns the entry boundary for admitted mutations rather than consuming
host-authored mutation translations as an external input.

This workstream should:

- add an engine-owned admitted mutation-entry surface for scalar lifecycle
  mutations and the already-admitted narrow structural mutations
- derive resident cell, lifetime, wiring, graph, and queue after-state from
  engine-owned entry requests
- preserve the current shadow/bootstrap builders as validation and import
  helpers
- keep the entry path value-semantic and free of hidden Calc pointer
  ownership

Required artifact:

- one checked-in implementation note for the engine-owned admitted-slice
  mutation-entry path

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_IMPLEMENTATION.md)

### 4. Build Calc Apply And Realization For Engine-Owned Mutation Entry

Teach Calc to consume engine-owned admitted mutation-entry output without
reclaiming authority for mutation intent or admitted after-state decisions.

This workstream should:

- add narrow compat adapters that apply engine-issued admitted mutation
  deltas into Calc
- preserve the current engine-owned resident cell, resident wiring, and
  formula-cell-lifetime paths
- keep Calc as the temporary owner of:
  - live object realization
  - final rollback
- ensure engine-owned mutation-entry state, not Calc-local reconstruction, is
  the thing deciding the admitted after-state

Required artifact:

- one checked-in implementation note for Calc apply/realization of
  engine-owned mutation-entry output

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_REALIZATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_REALIZATION.md)

### 5. Freeze Differential Mutation-Entry Evidence

Run the bounded proof cycle and record the results.

This evidence note should summarize:

- exact same-mutation comparisons between:
  - engine-owned admitted mutation-entry requests and after-state
  - Calc realized live state after engine-issued apply/realization
  - engine-owned resident cell state
  - engine-owned resident wiring state
  - engine-owned formula-cell lifetime state
  - engine-owned graph and queue state
- rollback-triggering cases
- repair-detected cases
- memory and performance observations
- whether engine-owned mutation entry is materially cleaner than the current
  engine-owned-state-plus-host-entry boundary

Required artifact:

- one checked-in mutation-entry evidence note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_EVIDENCE.md)

### 6. Freeze The Mutation-Entry Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-owned mutation entry on the admitted slice
- keep mutation entry validation-only
- defer mutation entry again

The decision record must also state the next adjacent concern after this
decision:

- broader object-realization migration
- broader dependency-container migration
- or another newly bounded reassessment if the mutation-entry proof does not
  hold

Required artifact:

- one checked-in mutation-entry decision record

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md)

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)
- [ComputationalSubstrateAuthority.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx)
- [ComputationalSubstrateLifecycle.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLifecycle.hxx)
- [ComputationalSubstrateStructural.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateStructural.hxx)
- [ComputationalSubstrateCellStorage.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx)
- [ComputationalSubstrateFormulaCellLifetime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx)
- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- a new admitted-slice mutation-entry compat adapter under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)

## Recommended Execution Order

The recommended order is:

1. freeze the admitted mutation-entry contract
2. freeze the engine-owned mutation-entry schema
3. build the engine-owned admitted mutation-entry path
4. build Calc apply/realization from engine-owned mutation-entry output
5. freeze the differential evidence
6. close with an explicit proceed / validation-only / defer decision

## Validation Contract

At minimum, each bounded implementation step should keep the following green:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

If new mutation-entry-specific proof lanes are added, they should be made
part of this standing contract before closeout.

## Exit Criteria

This plan is complete only if all of the following are true:

- the admitted mutation-entry contract is frozen and respected
- the engine-owned mutation-entry schema is explicit and stable
- the engine-owned admitted mutation-entry path is implemented and proven
  exact on the admitted slice
- Calc can apply and realize admitted live state from engine-owned
  mutation-entry output without reclaiming after-state authority
- exact computational, graph, and queue verification still hold on the
  admitted slice
- rollback and repair-detected behavior remain explicit and green
- the closeout decision says whether admitted-slice mutation entry:
  - proceeds
  - remains validation-only
  - or is deferred again

The plan is not complete merely because the engine stores more mutation
metadata. It is complete only when the admitted mutation-entry boundary is
either proven, explicitly limited to validation-only, or explicitly deferred.
