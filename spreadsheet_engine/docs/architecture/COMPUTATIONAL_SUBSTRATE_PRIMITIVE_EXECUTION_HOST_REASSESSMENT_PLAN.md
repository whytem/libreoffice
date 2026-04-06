# Computational Substrate Primitive Execution Host-Operation Reassessment Plan

Status: implementation-ready plan

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_DECISION_RECORD.md).

That closeout moved the admitted slice to its strongest proven position so
far:

- the engine owns admitted resident cell storage
- the engine owns admitted resident wiring containers
- the engine owns admitted formula-cell lifetime decisions
- the engine owns admitted scalar mutation-entry request shape, routing, and
  after-state decisions
- the engine owns admitted raw mutation records
- the engine owns admitted raw document mutation records and primitive apply
  verdicts
- the engine owns admitted live object-realization records
- the engine owns admitted rollback records
- the engine owns admitted live apply plans
- the engine owns admitted primitive realization and primitive rollback
  records and apply verdicts
- the engine owns admitted final verification records and observations

What still remains host-owned on that same slice is now narrower:

- the primitive execution host operations that still perform admitted
  low-level document mutation, realization, and rollback work
- any workbook or mutation class outside the admitted slice

This plan is not a broad `ScDocument` transplant. It is the next bounded
reassessment of whether the retained primitive execution shell can become
more explicitly engine-authored without reopening broader workbook-scope,
shared-group, named-range-sensitive, or host-service migration.

## Why This Is The Best Next Path

The project has already proven nearly all of the bounded admitted-slice live
authority surfaces that were the original blockers:

- resident cell storage
- resident wiring containers
- formula-cell lifetime decisions
- mutation-entry request and after-state decisions
- raw mutation identity
- raw document mutation identity
- live object-realization records
- rollback records
- live apply sequencing
- primitive realization and primitive rollback identity
- final verification identity

That means the most actionable remaining host-owned choke point on the
bounded slice is no longer storage, wiring, lifetime, mutation identity,
realization identity, rollback identity, or verification identity. It is
the primitive execution shell that still performs the underlying host
operations for that already-engine-authored admitted state.

The shortest path toward broader engine-owned live authority is therefore:

1. define one explicit engine-authored primitive execution plan for the
   admitted slice
2. keep Calc as the temporary primitive call host
3. preserve exact queue, computational, graph, replay, realization,
   rollback, and verification proof
4. decide only after that proof whether broader host-shell reduction is
   justified

## Plan Goal

Determine whether `spreadsheet_engine` can safely own more of the admitted
primitive execution shell on the admitted slice, so that Calc no longer
quietly performs admitted low-level mutation, realization, and rollback
operations through hidden local host sequencing after the engine-authored
raw mutation, realization, rollback, and verification records have already
been built.

The goal is to decide one explicit question:

- proceed with engine-authored admitted-slice primitive execution migration
- keep the primitive execution shell hybrid on the admitted slice
- or defer broader primitive execution migration again

## Entry Boundary

This plan begins from the current settled admitted-slice boundary:

- the first-stage extraction boundary is complete and stable
- the computational-substrate authority program closed with a narrow proceed
  result
- the narrow opt-in rollout remains bounded to the admitted scalar and
  single-sheet structural slice
- the engine already owns:
  - admitted resident cell storage
  - admitted resident wiring containers
  - admitted formula-cell lifetime decisions
  - admitted scalar mutation-entry request shape, routing, and after-state
    decisions
  - admitted raw mutation records
  - admitted raw document mutation records and primitive apply verdicts
  - admitted live object-realization records
  - admitted rollback records
  - admitted live apply plans
  - admitted primitive realization and primitive rollback records and apply
    verdicts
  - admitted final verification records and observations
- exact queue, computational, graph, replay, realization, rollback, and
  verification already hold on the admitted slice
- Calc still owns:
  - the primitive execution host operations around admitted mutation,
    realization, and rollback execution

This plan must therefore treat primitive execution as the next proof
surface, not as an already-admitted extension of the current boundary.

## Non-Goals

This plan should not attempt to:

- move UI, UNO, rendering, import/export, persistence, or environment
  services into the engine
- replace all `ScDocument` primitive execution behavior in one sweep
- widen beyond the admitted scalar lifecycle and admitted narrow structural
  slice
- widen into shared-group-sensitive or named-range-sensitive behavior
- widen into sheet insert, delete, rename, or move
- migrate token-container ownership
- reopen resident storage, resident wiring, lifetime, raw mutation, raw
  document mutation, live apply, primitive realization/rollback, or final
  verification migration
- weaken exact queue, computational, graph, replay, realization, rollback,
  or verification requirements
- claim broad `ScDocument` host independence

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted primitive execution
   surface
2. a checked-in schema note for the engine-authored primitive execution
   record or plan and verdicts
3. a checked-in observation and classification note for primitive execution
   drift
4. a checked-in implementation note for the engine-authored primitive
   execution path
5. a checked-in evidence note covering exact, reject, rollback, and
   deferred primitive execution outcomes
6. a checked-in decision record saying whether admitted-slice primitive
   execution migration:
   - proceeds
   - remains hybrid
   - or is deferred again

## Workstreams

### 1. Freeze The Primitive Execution Contract

Freeze the exact admitted primitive execution surface before implementation
work begins.

This contract should name:

- the admitted workbook and mutation classes
- the exact primitive execution classes under reassessment:
  - admitted low-level scalar overwrite and clear execution
  - admitted low-level formula replace execution
  - admitted low-level single-sheet row and column structural execution
  - admitted low-level realization and rollback primitive call sequencing
  - admitted runtime combinations of raw mutation, raw document mutation,
    live apply, primitive realization/rollback, final verification, and
    primitive execution
- retained Calc-owned host surfaces that stay out of scope in this cycle
- exact success criteria versus immediate hybrid or defer triggers

Required artifact:

- one checked-in primitive execution contract note

### 2. Freeze The Primitive Execution Schema

Define the representative admitted-slice primitive execution shape that the
proof cycle must use.

This schema note should define:

- engine-authored admitted primitive execution records or plans
- stable primitive execution identity, normalized payload rules, and
  verdict categories
- linkage between primitive execution and the admitted raw document
  mutation, primitive realization/rollback, and final verification records
- exact versus normalized-equivalent primitive execution outcomes
- forbidden Calc-local shortcuts that would reclaim primitive execution
  authority

Required artifact:

- one checked-in primitive execution schema note

### 3. Build The Primitive Execution Observation Path

Make remaining primitive execution gaps explicit enough to distinguish:

- exact engine-authored primitive execution
- ordering-only host execution differences
- hidden host primitive execution orchestration or repair
- missing primitive execution inputs caused by earlier shell drift
- true queue, computational, graph, replay, realization, rollback, or
  verification divergence

This workstream should:

- add bounded admitted-slice primitive execution proof lanes
- surface primitive execution differences explicitly in test output
- keep the observation layer scoped only to the admitted slice

Required artifact:

- one checked-in primitive execution observation and classification note

### 4. Build The Engine-Authored Primitive Execution Path

Implement the narrowest change set needed to make admitted primitive
execution more explicitly engine-authored on the bounded slice.

This workstream should:

- add stable engine-authored admitted primitive execution records or
  equivalent instructions
- teach Calc to consume those execution surfaces without reclaiming hidden
  primitive execution authority
- preserve engine-owned resident storage, resident wiring, lifetime, raw
  mutation, raw document mutation, live apply, primitive
  realization/rollback, and final verification decisions
- keep the broader live document shell in Calc for this cycle

Required artifact:

- one checked-in implementation note for the engine-authored primitive
  execution path

### 5. Freeze Differential Primitive Execution Evidence

Run the bounded primitive execution proof cycle and record the results.

This evidence note should summarize:

- exact admitted-slice comparisons between engine-authored primitive
  execution and the live Calc document after apply or reject
- whether resident cell state, resident wiring state, formula-cell
  lifetime, raw mutation, raw document mutation, realization, rollback,
  live apply, primitive realization/rollback, and final verification all
  continue to close exactly
- reject, rollback, and deferred cases
- memory and performance observations
- whether the host-owned primitive execution shell actually shrinks in a
  meaningful way

Required artifact:

- one checked-in primitive execution evidence note

### 6. Freeze The Primitive Execution Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-authored admitted-slice primitive execution migration
- keep the primitive execution shell hybrid on the admitted slice
- defer broader primitive execution migration again

The decision record must also state the next adjacent concern after this
closeout:

- another newly bounded primitive host-operation gap
- another newly bounded admitted-slice live host shell
- or another newly bounded adjacent concern if this proof does not hold

Required artifact:

- one checked-in primitive execution decision record

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [ComputationalSubstrateRawMutation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx)
- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)
- [ComputationalSubstrateRollback.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx)
- [ComputationalSubstrateCellStorage.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx)
- [ComputationalSubstrateFormulaCellLifetime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx)
- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Recommended Execution Order

Run the workstreams in this order:

1. freeze the admitted primitive execution contract
2. freeze the primitive execution schema
3. freeze the observation and classification model
4. implement the narrow engine-authored primitive execution path
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
- the admitted primitive execution proof lanes distinguish exact, reject,
  rollback, and deferred outcomes explicitly
- the closeout says clearly whether the admitted primitive execution shell:
  - proceeds
  - remains hybrid
  - or is deferred again
- the next adjacent concern is stated explicitly
- all standing validation lanes remain green
- the zero-fallback replay baseline remains exact
