# Computational Substrate Raw Document Mutation API Migration Plan

Status: implementation-ready plan

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_DECISION_RECORD.md).

That closeout moved the admitted slice to its strongest proven position so
far:

- the engine owns admitted resident cell storage
- the engine owns admitted resident wiring containers
- the engine owns admitted formula-cell lifetime decisions
- the engine owns admitted scalar mutation-entry request shape, routing, and
  after-state decisions
- the engine owns admitted live object-realization records
- the engine owns admitted rollback records
- the engine owns admitted raw mutation records consumed before live apply
- the engine owns admitted live apply plans consumed before exact
  verification completes

What still remains host-owned on that same slice is now narrower:

- the underlying raw document mutation APIs used to execute the admitted
  plan stages
- the primitive realization and rollback host operations around those APIs
- the final verification host shell around those primitive operations

This plan is not a broad `ScDocument` transplant. It is the next bounded
reassessment of whether the admitted raw document mutation shell can become
more explicitly engine-authored without reopening broader workbook-scope,
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
- raw mutation identity
- live apply sequencing

That means the most actionable remaining host-owned choke point on the
bounded slice is no longer storage, wiring, lifetime, rollback, mutation
identity, or apply-shell sequencing. It is the underlying raw document
mutation shell that still executes the admitted plan.

The shortest path toward broader engine-owned live authority is therefore:

1. define one explicit engine-authored primitive mutation shell for the
   admitted slice
2. keep Calc as the temporary primitive realization, rollback, and
   verification host
3. preserve exact queue, computational, graph, replay, realization, and
   rollback proof
4. decide only after that proof whether broader host-shell reduction is
   justified

## Plan Goal

Determine whether `spreadsheet_engine` can safely own more of the admitted
raw document mutation shell on the admitted slice, so that Calc no longer
quietly executes admitted scalar and narrow structural mutations through
hidden local mutation APIs before the engine-authored live apply plan
completes.

The goal is to decide one explicit question:

- proceed with engine-authored admitted-slice raw document mutation API
  migration
- keep the raw document mutation shell hybrid on the admitted slice
- or defer broader raw document mutation migration again

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
- exact queue, computational, graph, replay, realization, and rollback
  verification already holds on the admitted slice
- Calc still owns:
  - the underlying raw document mutation APIs used to execute the admitted
    plan stages
  - the primitive realization and rollback host operations
  - the final verification host shell

This plan must therefore treat primitive mutation execution as the next proof
surface, not as an already-admitted extension of the current boundary.

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
- reopen resident storage, resident wiring, lifetime, raw mutation,
  realization, rollback, or live apply migration
- weaken exact queue, computational, graph, replay, realization, or
  rollback requirements
- claim broad `ScDocument` host independence

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted raw document mutation
   shell surface
2. a checked-in schema note for the engine-authored primitive mutation
   record and verdict records
3. a checked-in observation and classification note for raw document
   mutation-shell divergence
4. a checked-in implementation note for the engine-authored raw document
   mutation path
5. a checked-in evidence note covering exact, rollback, and deferred raw
   document mutation outcomes
6. a checked-in decision record saying whether admitted-slice raw document
   mutation migration:
   - proceeds
   - remains hybrid
   - or is deferred again

## Workstreams

### 1. Freeze The Raw Document Mutation Contract

Freeze the exact admitted primitive mutation surface before implementation
work begins.

This contract should name:

- the admitted workbook and mutation classes
- the exact primitive mutation classes under reassessment:
  - admitted scalar overwrite and scalar text or number assignment
  - admitted formula replace and clear
  - admitted single-sheet row and column structural mutations
  - admitted runtime combinations of primitive mutation execution plus
    realization, verification, and rollback record consumption
- retained Calc-owned host surfaces that stay out of scope in this cycle
- exact success criteria versus immediate hybrid or defer triggers

Required artifact:

- one checked-in raw document mutation contract note

### 2. Freeze The Raw Document Mutation Schema

Define the representative admitted-slice primitive mutation and verdict shape
that the proof cycle must use.

This schema note should define:

- engine-authored admitted primitive mutation records
- stable mutation identity and normalized payload rules
- linkage between primitive mutation execution and the admitted live apply
  plan
- exact versus normalized-equivalent primitive mutation outcomes
- forbidden Calc-local shortcuts that would reclaim primitive mutation
  authority

Required artifact:

- one checked-in raw document mutation schema note

### 3. Build The Raw Document Mutation Observation And Classification Path

Make remaining primitive mutation gaps explicit enough to distinguish:

- exact engine-authored primitive mutation execution
- ordering-only host execution differences
- hidden host mutation orchestration or repair
- missing realized or rolled-back live objects caused by primitive mutation
  drift
- true queue, computational, graph, or replay divergence

This workstream should:

- add bounded admitted-slice primitive mutation proof lanes
- surface primitive mutation differences explicitly in test output
- keep the observation layer scoped only to the admitted slice

Required artifact:

- one checked-in raw document mutation observation and classification note

### 4. Build The Engine-Authored Raw Document Mutation Path

Implement the narrowest change set needed to make admitted primitive
mutation execution more explicitly engine-authored on the bounded slice.

This workstream should:

- add a stable engine-authored admitted primitive mutation record or
  equivalent primitive mutation instructions
- teach Calc to consume that primitive mutation surface without reclaiming
  hidden mutation execution authority
- preserve engine-owned resident storage, resident wiring, lifetime, raw
  mutation, realization, rollback, and live apply decisions
- keep primitive realization and rollback host operations in Calc for this
  cycle

Required artifact:

- one checked-in implementation note for the engine-authored raw document
  mutation path

### 5. Freeze Differential Raw Document Mutation Evidence

Run the bounded primitive mutation proof cycle and record the results.

This evidence note should summarize:

- exact admitted-slice comparisons between engine-authored primitive
  mutation execution and the live Calc document after apply
- whether resident cell state, resident wiring state, formula-cell lifetime,
  raw mutation, realization, rollback, and live apply all continue to close
  exactly
- repair-detected and reject cases
- memory and performance observations
- whether the host-owned raw document mutation shell actually shrinks in a
  meaningful way

Required artifact:

- one checked-in raw document mutation evidence note

### 6. Freeze The Raw Document Mutation Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-authored admitted-slice raw document mutation API
  migration
- keep the raw document mutation shell hybrid on the admitted slice
- defer broader raw document mutation migration again

The decision record must also state the next adjacent concern after this
closeout:

- broader admitted-slice host-shell reduction
- primitive realization or rollback shell reassessment
- or another newly bounded raw document mutation gap if this proof does not
  hold

Required artifact:

- one checked-in raw document mutation decision record

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [ComputationalSubstrateRawMutation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx)
- [ComputationalSubstrateLiveApply.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLiveApply.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)
- [ComputationalSubstrateRollback.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Recommended Execution Order

Run the workstreams in this order:

1. freeze the admitted raw document mutation contract
2. freeze the primitive mutation schema
3. freeze the observation and classification model
4. implement the narrow engine-authored primitive mutation path
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
- the admitted primitive mutation proof lanes distinguish exact, rollback,
  and deferred outcomes explicitly
- the closeout says clearly whether the admitted raw document mutation shell:
  - proceeds
  - remains hybrid
  - or is deferred again
- the next adjacent concern is stated explicitly
- all standing validation lanes remain green
- the zero-fallback replay baseline remains exact
