# Admitted Slice Ownership Closeout Plan

Status: implementation-ready plan

## Purpose

This document defines the endgame plan for finishing substantive ownership
of the current admitted computational-substrate slice inside
`spreadsheet_engine/`.

The goal is narrow and explicit:

- finish the remaining host-owned shell on the current admitted slice
- keep the admitted slice itself fixed while that ownership is completed
- avoid mixing slice-completion work with broader workbook-class widening

This is not a plan for shared-group widening, named-range-sensitive
structural widening, sheet-wide mutation migration, or broad `ScDocument`
independence. It is the closeout plan for the already-admitted slice.

## Why This Plan Exists

The project has already justified substantive engine ownership on the
bounded admitted slice for:

- resident cell storage
- resident wiring containers
- formula-cell lifetime decisions
- mutation-entry request shape, routing, and after-state decisions
- raw mutation identity
- raw document mutation identity
- live object-realization identity
- rollback identity
- live apply identity
- primitive realization and primitive rollback identity
- final verification identity
- primitive execution identity and stage sequencing

The remaining host-owned concern on that same slice is now narrower:

- the primitive host-call executor that still performs the low-level
  admitted mutation, realization, and rollback calls

That means the remaining work to complete ownership on the current slice is
now one coherent endgame program rather than multiple unrelated migration
fronts.

## Plan Goal

Determine whether `spreadsheet_engine` can safely complete substantive
ownership of the current admitted slice by making the retained primitive
host-call executor on that slice explicitly engine-authored and fully
bounded.

The closeout must answer one explicit question:

- admitted-slice ownership is complete
- admitted-slice ownership remains hybrid for one bounded reason
- or admitted-slice ownership closeout must be deferred again

## Entry Boundary

This plan begins from the current settled admitted-slice boundary:

- the first-stage extraction boundary is complete and stable
- the computational-substrate authority program closed with a narrow proceed
  result
- the admitted narrow rollout remains bounded to the scalar and single-sheet
  structural slice
- the engine already owns on that slice:
  - resident cell storage
  - resident wiring containers
  - formula-cell lifetime decisions
  - scalar mutation-entry request shape, routing, and after-state decisions
  - raw mutation records
  - raw document mutation records and primitive apply verdicts
  - live object-realization records
  - rollback records
  - live apply plans
  - primitive realization and primitive rollback records and apply verdicts
  - final verification records and observations
  - primitive execution plans and observations
- exact queue, computational, graph, replay, realization, rollback, and
  verification already hold on the admitted slice
- Calc still owns:
  - the primitive host-call executor around admitted low-level mutation,
    realization, and rollback work
  - all workbook and mutation classes outside the admitted slice

This plan therefore treats primitive host-call execution as the final
ownership-completion surface for the current slice.

## In-Scope Slice

The admitted slice remains fixed throughout this plan:

- ordinary scalar formulas only
- clean baseline only
- `SetScalarValue`
- `SetFormula`
- `ClearCell`
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`
- no shared groups
- no named-range-sensitive structural behavior
- no sheet insert, delete, rename, or move
- no copy, move, clipboard, load-time, or undo-like flows

If the workbook or mutation leaves that slice, the plan must reject,
rollback, or defer rather than silently widening.

## Non-Goals

This plan should not attempt to:

- widen the admitted workbook or mutation surface
- move UI, UNO, rendering, import/export, persistence, or environment
  services into the engine
- replace all `ScDocument` execution behavior in one sweep
- widen into shared-group-sensitive behavior
- widen into named-range-sensitive behavior
- widen into sheet-wide or document-wide structural mutation
- migrate token-container ownership
- reopen already-settled ownership of resident storage, resident wiring,
  lifetime, mutation-entry, realization, rollback, or verification identity
- weaken exact queue, computational, graph, replay, realization, rollback,
  or verification requirements

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted-slice ownership-closeout
   boundary
2. a checked-in schema note for the primitive host-call executor plan and
   verdict surfaces
3. a checked-in observation note for primitive host-call execution drift
4. a checked-in implementation note for the engine-authored primitive
   host-call executor path
5. a checked-in evidence note covering exact, reject, rollback, and
   deferred ownership-closeout outcomes
6. a checked-in decision record saying whether admitted-slice ownership:
   - is complete
   - remains hybrid for one bounded reason
   - or is deferred again

## Workstreams

### 1. Freeze The Ownership-Closeout Contract

Freeze the exact admitted ownership-completion surface before implementation
work begins.

This contract should name:

- the admitted workbook and mutation classes
- the exact retained primitive host-call execution classes under
  reassessment:
  - low-level scalar overwrite and clear calls
  - low-level formula replace calls
  - low-level single-sheet row and column structural calls
  - low-level realization calls
  - low-level rollback calls
  - admitted runtime combinations of raw mutation, raw document mutation,
    primitive execution, realization, rollback, live apply, and final
    verification
- retained Calc-owned host surfaces that stay out of scope in this cycle
- the explicit definition of “ownership complete” on the admitted slice
- exact success criteria versus immediate hybrid or defer triggers

Required artifact:

- one checked-in ownership-closeout contract note

### 2. Freeze The Primitive Host-Call Executor Schema

Define the representative admitted-slice executor shape that the proof cycle
must use.

This schema note should define:

- engine-authored admitted primitive host-call executor plans
- stable executor identity and stage linkage
- linkage between executor plans and the settled raw document mutation,
  primitive realization or rollback, and final verification records
- executor verdict categories
- exact versus normalized-equivalent executor outcomes if any normalized
  class is still needed
- forbidden Calc-local shortcuts that would reclaim executor authority

Required artifact:

- one checked-in executor schema note

### 3. Build The Primitive Host-Call Observation Path

Make remaining primitive host-call gaps explicit enough to distinguish:

- exact engine-authored executor behavior
- ordering-only host-call drift
- hidden host execution or repair
- missing execution inputs caused by earlier shell drift
- true queue, computational, graph, replay, realization, rollback, or
  verification divergence

This workstream should:

- add bounded admitted-slice executor proof lanes
- surface executor differences explicitly in test output
- keep the observation layer scoped only to the admitted slice

Required artifact:

- one checked-in executor observation and classification note

### 4. Build The Engine-Authored Primitive Host-Call Executor Path

Implement the narrowest change set needed to make admitted primitive
host-call execution explicitly engine-authored on the bounded slice.

This workstream should:

- add stable engine-authored admitted primitive host-call executor plans or
  equivalent instructions
- route admitted apply lanes through that executor surface
- route admitted rollback lanes through that same executor surface
- teach Calc to consume those execution surfaces without reclaiming hidden
  executor authority
- preserve engine-owned resident storage, resident wiring, lifetime,
  mutation-entry, raw mutation, raw document mutation, live apply,
  realization, rollback, final verification, and primitive execution
  decisions

Required artifact:

- one checked-in implementation note for the engine-authored primitive
  host-call executor path

### 5. Freeze Differential Ownership-Closeout Evidence

Run the bounded ownership-closeout proof cycle and record the results.

This evidence note should summarize:

- exact admitted-slice comparisons between engine-authored executor plans
  and the live Calc document after apply or reject
- whether resident cell state, resident wiring state, formula-cell
  lifetime, raw mutation, raw document mutation, realization, rollback,
  live apply, primitive execution, and final verification all continue to
  close exactly
- reject, rollback, and deferred cases
- memory and performance observations
- whether the host-owned executor shell actually shrinks in a meaningful
  way

Required artifact:

- one checked-in ownership-closeout evidence note

### 6. Freeze The Ownership-Closeout Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- admitted-slice ownership is complete
- admitted-slice ownership remains hybrid for one bounded reason
- admitted-slice ownership closeout is deferred again

The decision record must also state the next roadmap category after this
closeout:

- wider workbook-class expansion
- another bounded adjacent host shell if the current slice is still not
  complete
- or defer if the admitted slice still cannot be closed honestly

Required artifact:

- one checked-in ownership-closeout decision record

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [ComputationalSubstrateRawMutation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx)
- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)
- [ComputationalSubstrateRollback.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx)
- [ComputationalSubstrateCellStorage.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx)
- [ComputationalSubstrateFormulaCellLifetime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx)
- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- [ComputationalSubstratePrimitiveExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstratePrimitiveExecution.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Recommended Execution Order

Run the workstreams in this order:

1. freeze the ownership-closeout contract
2. freeze the executor schema
3. freeze the executor observation model
4. implement the engine-authored primitive host-call executor path
5. freeze differential evidence from exact, reject, and rollback lanes
6. close with a decision record and update the status/index docs

## Validation Contract

The plan should be considered valid only if the following stay green as the
work progresses:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

The closeout must continue to preserve the standing replay baseline at:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

## Exit Criteria

This plan is complete only when all of the following are true:

- the contract, schema, observation, implementation, evidence, and decision
  artifacts all exist
- the admitted executor proof lanes distinguish exact, reject, rollback,
  and deferred outcomes explicitly
- the closeout says clearly whether admitted-slice ownership:
  - is complete
  - remains hybrid
  - or is deferred again
- the next roadmap category is stated explicitly
- all standing validation lanes remain green
- the zero-fallback replay baseline remains exact
