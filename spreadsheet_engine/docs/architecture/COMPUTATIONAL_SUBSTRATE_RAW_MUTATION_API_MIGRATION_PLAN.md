# Computational Substrate Raw Mutation API Migration Plan

Status: implementation-ready plan

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_DECISION_RECORD.md).

That closeout moved the admitted slice to its strongest proven position so
far:

- the engine owns admitted resident cell storage
- the engine owns admitted resident wiring containers
- the engine owns admitted formula-cell lifetime decisions
- the engine owns admitted scalar mutation-entry request shape, routing, and
  after-state decisions
- the engine owns admitted live object-realization records
- the engine owns admitted rollback records

What still remains host-owned on that same slice is now narrower:

- raw document mutation APIs
- the final live apply shell that executes engine-authored realization and
  rollback records

This plan is not a broad document-host transplant. It is the next bounded
reassessment of whether admitted-slice raw mutation entry can move further
toward engine-authored authority without reopening broader workbook-scope,
shared-group, named-range-sensitive, or host-service migration.

## Why This Is The Best Next Path

The project has already proven nearly all of the bounded admitted-slice live
state surfaces that were the original blockers:

- resident cell storage
- resident wiring containers
- formula-cell lifetime decisions
- mutation-entry request and after-state decisions
- live object-realization records
- rollback records

That means the most actionable remaining host-owned choke point on the
bounded slice is no longer storage, wiring, realization, or rollback
authority. It is the raw mutation shell that still decides how admitted
mutations first enter Calc before the engine-owned records are consumed.

The shortest path toward broader engine-owned live authority is therefore:

1. move admitted raw mutation entry through an explicit engine-authored shell
2. keep Calc as the temporary live apply and verification host
3. preserve exact queue, computational, graph, replay, realization, and
   rollback proof
4. decide only after that proof whether broader host-shell reduction is
   justified

## Plan Goal

Determine whether `spreadsheet_engine` can safely own more of the admitted
raw mutation entry shape on the admitted slice, so that Calc no longer
quietly originates admitted scalar and narrow structural mutations through
hidden local authority before engine-owned realization and rollback records
are applied.

The goal is to decide one explicit question:

- proceed with engine-authored admitted-slice raw mutation API migration
- keep the raw mutation shell hybrid on the admitted slice
- or defer broader raw mutation migration again

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
  - admitted live object-realization records
  - admitted rollback records
- exact queue, computational, graph, replay, realization, and rollback
  verification already holds on the admitted scalar mutation-entry slice
- Calc still owns:
  - raw document mutation APIs
  - the final live apply shell that executes engine-authored realization and
    rollback records

This plan must therefore treat raw mutation entry as the next proof surface,
not as an already-admitted extension of the current boundary.

## Non-Goals

This plan should not attempt to:

- move UI, UNO, rendering, import/export, persistence, or environment
  services into the engine
- replace all `ScDocument` mutation APIs in one sweep
- widen beyond the admitted scalar lifecycle and admitted narrow structural
  slice
- widen into shared-group-sensitive or named-range-sensitive behavior
- widen into sheet insert, delete, rename, or move
- migrate token-container ownership
- reopen resident storage, resident wiring, realization, or rollback
  migration
- weaken exact queue, computational, graph, replay, realization, or
  rollback requirements
- claim broad `ScDocument` host independence

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted raw mutation shell
   surface
2. a checked-in schema note for engine-authored raw mutation request and
   verdict records
3. a checked-in observation and classification note for raw mutation shell
   divergence
4. a checked-in implementation note for the engine-authored raw mutation
   entry path
5. a checked-in evidence note covering exact, rollback, and deferred raw
   mutation outcomes
6. a checked-in decision record saying whether admitted-slice raw mutation
   migration:
   - proceeds
   - remains hybrid
   - or is deferred again

## Workstreams

### 1. Freeze The Raw Mutation Contract

Freeze the exact admitted raw mutation surface before implementation work
begins.

This contract should name:

- the admitted workbook and mutation classes
- the exact raw mutation entry classes under reassessment:
  - admitted scalar overwrite and scalar text or number assignment
  - admitted formula replace and clear
  - admitted single-sheet row and column structural mutations
  - admitted runtime combinations of raw mutation entry plus realization and
    rollback record consumption
- retained Calc-owned host surfaces that stay out of scope in this cycle
- exact success criteria versus immediate hybrid or defer triggers

Required artifact:

- one checked-in raw mutation contract note

### 2. Freeze The Raw Mutation Schema

Define the representative admitted-slice raw mutation request and verdict
shape that the proof cycle must use.

This schema note should define:

- engine-authored admitted raw mutation request records
- stable mutation identity and normalized payload rules
- before-state and after-state linkage to realization and rollback records
- exact versus normalized-equivalent mutation shell outcomes
- forbidden Calc-local shortcuts that would reclaim mutation authority

Required artifact:

- one checked-in raw mutation schema note

### 3. Build The Raw Mutation Observation And Classification Path

Make remaining raw mutation shell gaps explicit enough to distinguish:

- exact engine-authored mutation entry
- ordering-only host apply differences
- hidden host-originated mutation reconstruction
- missing realized or rolled-back live objects caused by shell drift
- true queue, computational, graph, or replay divergence

This workstream should:

- add bounded admitted-slice raw mutation proof lanes
- surface shell differences explicitly in test output
- keep the observation layer scoped only to the admitted slice

Required artifact:

- one checked-in raw mutation observation and classification note

### 4. Build The Engine-Authored Raw Mutation Path

Implement the narrowest change set needed to make admitted raw mutation entry
more explicitly engine-authored on the bounded slice.

This workstream should:

- add stable engine-authored raw mutation records or equivalent admitted
  mutation instructions
- teach Calc to consume that raw mutation surface without reclaiming hidden
  mutation intent authority
- preserve engine-owned resident storage, resident wiring, lifetime,
  realization, and rollback decisions
- keep the final live apply shell in Calc for this cycle

Required artifact:

- one checked-in implementation note for the engine-authored raw mutation
  path

### 5. Freeze Differential Raw Mutation Evidence

Run the bounded raw mutation proof cycle and record the results.

This evidence note should summarize:

- exact admitted-slice comparisons between engine-authored raw mutation entry
  and the live Calc document after apply
- whether resident cell state, resident wiring state, formula-cell lifetime,
  realization, and rollback all continue to close exactly
- repair-detected and reject cases
- memory and performance observations
- whether the host-owned mutation shell actually shrinks in a meaningful way

Required artifact:

- one checked-in raw mutation evidence note

### 6. Freeze The Raw Mutation Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-authored admitted-slice raw mutation migration
- keep raw mutation hybrid on the admitted slice
- defer broader raw mutation migration again

The decision record must also state the next adjacent concern after this
closeout:

- broader live apply-shell reassessment
- broader admitted-slice host-shell reduction
- or another newly bounded raw mutation gap if this proof does not hold

Required artifact:

- one checked-in raw mutation decision record

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)
- [ComputationalSubstrateRollback.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx)
- [ComputationalSubstrateCellStorage.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx)
- [ComputationalSubstrateFormulaCellLifetime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx)
- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)
- one new admitted-slice raw mutation compat surface under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- [workbook_facade_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/workbook_facade_tests.cxx)

## Recommended Execution Order

The recommended order is:

1. freeze the raw mutation contract
2. freeze the raw mutation schema
3. build the observation and classification path
4. build the engine-authored raw mutation path
5. freeze the differential evidence
6. close with an explicit proceed / hybrid / defer decision

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

If new raw-mutation-specific proof lanes are added, they should become part
of this standing contract before closeout.

## Exit Criteria

This plan is complete only if all of the following are true:

- the admitted raw mutation contract is frozen and respected
- the raw mutation schema and observation model are explicit and stable
- the engine-authored admitted raw mutation path is implemented or explicitly
  limited
- Calc can consume engine-authored raw mutation records on the admitted slice
  without reclaiming hidden mutation authority
- exact queue, computational, graph, replay, realization, and rollback
  verification still hold on the admitted slice
- repair-detected and reject behavior remain explicit and green
- the closeout decision says whether admitted-slice raw mutation migration:
  - proceeds
  - remains hybrid
  - or is deferred again

The plan is not complete merely because the engine stores more mutation
metadata. It is complete only when the admitted raw mutation boundary is
either proven, explicitly limited, or explicitly deferred.
