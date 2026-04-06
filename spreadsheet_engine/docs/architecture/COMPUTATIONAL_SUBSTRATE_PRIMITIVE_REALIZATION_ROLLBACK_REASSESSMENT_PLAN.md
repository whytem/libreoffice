# Computational Substrate Primitive Realization And Rollback Shell Reassessment Plan

Status: completed closeout record

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_DECISION_RECORD.md).

That closeout moved the admitted slice to its strongest proven position so
far:

- the engine owns admitted resident cell storage
- the engine owns admitted resident wiring containers
- the engine owns admitted formula-cell lifetime decisions
- the engine owns admitted scalar mutation-entry request shape, routing, and
  after-state decisions
- the engine owns admitted live object-realization records
- the engine owns admitted rollback records
- the engine owns admitted raw mutation records
- the engine owns admitted live apply plans
- the engine owns admitted raw document mutation records and primitive apply
  verdicts

What still remains host-owned on that same slice is now narrower:

- the primitive realization host operations around admitted primitive
  mutation execution
- the primitive rollback host operations around admitted primitive mutation
  execution
- the final verification host shell around those primitive operations

This plan is not a broad `ScDocument` transplant. It is the next bounded
reassessment of whether the admitted primitive realization and rollback
shell can become more explicitly engine-authored without reopening broader
workbook-scope, shared-group, named-range-sensitive, or host-service
migration.

## Why This Is The Best Next Path

The project has already proven nearly all of the bounded admitted-slice live
state surfaces that were the original blockers:

- resident cell storage
- resident wiring containers
- formula-cell lifetime decisions
- mutation-entry request and after-state decisions
- live object-realization records
- rollback records
- raw mutation identity
- live apply sequencing
- primitive raw document mutation identity

That means the most actionable remaining host-owned choke point on the
bounded slice is no longer storage, wiring, lifetime, mutation identity,
live apply sequencing, or primitive mutation identity. It is the primitive
realization and rollback shell that still executes the admitted engine-owned
state.

The shortest path toward broader engine-owned live authority is therefore:

1. define one explicit engine-authored primitive realization and rollback
   shell for the admitted slice
2. keep Calc as the temporary final verification host
3. preserve exact queue, computational, graph, replay, realization, and
   rollback proof
4. decide only after that proof whether broader host-shell reduction is
   justified

## Plan Goal

Determine whether `spreadsheet_engine` can safely own more of the admitted
primitive realization and rollback shell on the admitted slice, so that Calc
no longer quietly executes admitted realization and rollback through hidden
local host operations after the engine-authored raw document mutation and
live apply plan have already been built.

The goal is to decide one explicit question:

- proceed with engine-authored admitted-slice primitive realization and
  rollback shell migration
- keep the primitive realization and rollback shell hybrid on the admitted
  slice
- or defer broader primitive realization and rollback migration again

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
  - admitted raw mutation records
  - admitted live apply plans
  - admitted raw document mutation records and primitive apply verdicts
- exact queue, computational, graph, replay, realization, and rollback
  verification already holds on the admitted slice
- Calc still owns:
  - the primitive realization host operations around admitted primitive
    execution
  - the primitive rollback host operations around admitted primitive
    execution
  - the final verification host shell

This plan must therefore treat primitive realization and rollback execution
as the next proof surface, not as an already-admitted extension of the
current boundary.

## Non-Goals

This plan should not attempt to:

- move UI, UNO, rendering, import/export, persistence, or environment
  services into the engine
- replace all `ScDocument` realization and rollback behavior in one sweep
- widen beyond the admitted scalar lifecycle and admitted narrow structural
  slice
- widen into shared-group-sensitive or named-range-sensitive behavior
- widen into sheet insert, delete, rename, or move
- migrate token-container ownership
- reopen resident storage, resident wiring, lifetime, raw mutation, raw
  document mutation, or live apply migration
- weaken exact queue, computational, graph, replay, realization, or
  rollback requirements
- claim broad `ScDocument` host independence

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted primitive realization
   and rollback shell surface
2. a checked-in schema note for the engine-authored primitive realization
   and rollback records and verdicts
3. a checked-in observation and classification note for primitive
   realization and rollback shell divergence
4. a checked-in implementation note for the engine-authored primitive
   realization and rollback path
5. a checked-in evidence note covering exact, rollback, and deferred
   primitive realization and rollback outcomes
6. a checked-in decision record saying whether admitted-slice primitive
   realization and rollback migration:
   - proceeds
   - remains hybrid
   - or is deferred again

## Workstreams

### 1. Freeze The Primitive Realization And Rollback Contract

Freeze the exact admitted primitive realization and rollback surface before
implementation work begins.

This contract should name:

- the admitted workbook and mutation classes
- the exact primitive realization and rollback classes under reassessment:
  - admitted formula-cell lifetime realization
  - admitted resident cell-storage mirroring consumed during realization
  - admitted wiring realization consumed during realization
  - admitted rollback restore of those realized objects and resident state
  - admitted runtime combinations of primitive realization and rollback plus
    exact verification record consumption
- retained Calc-owned host surfaces that stay out of scope in this cycle
- exact success criteria versus immediate hybrid or defer triggers

Required artifact:

- one checked-in primitive realization and rollback contract note

### 2. Freeze The Primitive Realization And Rollback Schema

Define the representative admitted-slice primitive realization and rollback
shape that the proof cycle must use.

This schema note should define:

- engine-authored admitted primitive realization records
- engine-authored admitted primitive rollback records
- stable realization and rollback identity and normalized payload rules
- linkage between primitive realization and rollback execution and the
  admitted live apply plan
- exact versus normalized-equivalent primitive realization and rollback
  outcomes
- forbidden Calc-local shortcuts that would reclaim primitive realization or
  rollback authority

Required artifact:

- one checked-in primitive realization and rollback schema note

### 3. Build The Primitive Realization And Rollback Observation Path

Make remaining primitive realization and rollback gaps explicit enough to
distinguish:

- exact engine-authored primitive realization and rollback execution
- ordering-only host execution differences
- hidden host realization or rollback orchestration or repair
- missing realized or restored live objects caused by primitive shell drift
- true queue, computational, graph, or replay divergence

This workstream should:

- add bounded admitted-slice primitive realization and rollback proof lanes
- surface primitive realization and rollback differences explicitly in test
  output
- keep the observation layer scoped only to the admitted slice

Required artifact:

- one checked-in primitive realization and rollback observation and
  classification note

### 4. Build The Engine-Authored Primitive Realization And Rollback Path

Implement the narrowest change set needed to make admitted primitive
realization and rollback execution more explicitly engine-authored on the
bounded slice.

This workstream should:

- add stable engine-authored admitted primitive realization and rollback
  records or equivalent instructions
- teach Calc to consume those primitive realization and rollback surfaces
  without reclaiming hidden execution authority
- preserve engine-owned resident storage, resident wiring, lifetime, raw
  mutation, raw document mutation, and live apply decisions
- keep final verification host operations in Calc for this cycle

Required artifact:

- one checked-in implementation note for the engine-authored primitive
  realization and rollback path

### 5. Freeze Differential Primitive Realization And Rollback Evidence

Run the bounded primitive realization and rollback proof cycle and record
the results.

This evidence note should summarize:

- exact admitted-slice comparisons between engine-authored primitive
  realization and rollback execution and the live Calc document after apply
  or restore
- whether resident cell state, resident wiring state, formula-cell lifetime,
  raw mutation, raw document mutation, realization, rollback, and live apply
  all continue to close exactly
- repair-detected and reject cases
- memory and performance observations
- whether the host-owned primitive realization and rollback shell actually
  shrinks in a meaningful way

Required artifact:

- one checked-in primitive realization and rollback evidence note

### 6. Freeze The Primitive Realization And Rollback Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-authored admitted-slice primitive realization and
  rollback migration
- keep the primitive realization and rollback shell hybrid on the admitted
  slice
- defer broader primitive realization and rollback migration again

The decision record must also state the next adjacent concern after this
closeout:

- final verification host-shell reassessment
- another newly bounded primitive host-shell gap
- or another newly bounded adjacent concern if this proof does not hold

Required artifact:

- one checked-in primitive realization and rollback decision record

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)
- [ComputationalSubstrateRollback.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx)
- [ComputationalSubstrateLiveApply.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLiveApply.hxx)
- [ComputationalSubstrateRawMutation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Recommended Execution Order

Run the workstreams in this order:

1. freeze the admitted primitive realization and rollback contract
2. freeze the primitive realization and rollback schema
3. freeze the observation and classification model
4. implement the narrow engine-authored primitive realization and rollback
   path
5. freeze differential evidence from exact and rollback lanes
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
- the admitted primitive realization and rollback proof lanes distinguish
  exact, rollback, and deferred outcomes explicitly
- the closeout says clearly whether the admitted primitive realization and
  rollback shell:
  - proceeds
  - remains hybrid
  - or is deferred again
- the next adjacent concern is stated explicitly
- all standing validation lanes remain green
- the zero-fallback replay baseline remains exact
