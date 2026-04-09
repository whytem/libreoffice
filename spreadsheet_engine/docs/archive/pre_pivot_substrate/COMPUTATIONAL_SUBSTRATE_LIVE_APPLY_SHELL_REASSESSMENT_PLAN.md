# Computational Substrate Live Apply-Shell Reassessment Plan

Status: completed closeout record

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_DECISION_RECORD.md).

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

What still remains host-owned on that same slice is now narrower:

- the underlying raw document mutation APIs used to execute admitted records
- the final live apply shell that sequences engine-authored raw mutation,
  realization, verification, and rollback records

This plan is not a broad `ScDocument` transplant. It is the next bounded
reassessment of whether the admitted live apply shell can become more
explicitly engine-authored without reopening broader workbook-scope,
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

That means the most actionable remaining host-owned choke point on the
bounded slice is no longer storage, wiring, lifetime, rollback, or mutation
identity. It is the final apply shell that still decides how those
engine-authored records are sequenced and consumed at runtime.

The shortest path toward broader engine-owned live authority is therefore:

1. define one explicit engine-authored apply plan for the admitted slice
2. keep Calc as the temporary primitive executor and verification host
3. preserve exact queue, computational, graph, replay, realization, and
   rollback proof
4. decide only after that proof whether broader host-shell reduction is
   justified

## Plan Goal

Determine whether `spreadsheet_engine` can safely own more of the admitted
live apply-shell sequencing on the admitted slice, so that Calc no longer
quietly coordinates admitted raw mutation, realization, and rollback records
through hidden local orchestration before exact verification completes.

The goal is to decide one explicit question:

- proceed with an engine-authored admitted-slice live apply plan
- keep the live apply shell hybrid on the admitted slice
- or defer broader live apply-shell migration again

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
- exact queue, computational, graph, replay, realization, and rollback
  verification already holds on the admitted slice
- Calc still owns:
  - the raw document mutation APIs used to execute the admitted records
  - the final live apply shell that sequences those records

This plan must therefore treat live apply-shell sequencing as the next proof
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
- reopen resident storage, resident wiring, lifetime, raw mutation, or
  rollback migration
- weaken exact queue, computational, graph, replay, realization, or
  rollback requirements
- claim broad `ScDocument` host independence

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted live apply-shell surface
2. a checked-in schema note for the engine-authored apply-plan record and
   verdict records
3. a checked-in observation and classification note for live apply-shell
   divergence
4. a checked-in implementation note for the engine-authored live apply-plan
   path
5. a checked-in evidence note covering exact, rollback, and deferred
   apply-shell outcomes
6. a checked-in decision record saying whether admitted-slice live
   apply-shell migration:
   - proceeds
   - remains hybrid
   - or is deferred again

## Workstreams

### 1. Freeze The Live Apply-Shell Contract

Freeze the exact admitted live apply-shell surface before implementation work
begins.

This contract should name:

- the admitted workbook and mutation classes
- the exact apply-shell classes under reassessment:
  - admitted raw mutation record consumption
  - admitted realization record consumption
  - admitted exact verification sequencing
  - admitted rollback record consumption
  - admitted runtime combinations of those stages on scalar and narrow
    structural lanes
- retained Calc-owned host surfaces that stay out of scope in this cycle
- exact success criteria versus immediate hybrid or defer triggers

Required artifact:

- one checked-in live apply-shell contract note

### 2. Freeze The Live Apply-Shell Schema

Define the representative admitted-slice apply-plan and verdict shape that
the proof cycle must use.

This schema note should define:

- engine-authored admitted apply-plan records
- stage ordering and required stage identity rules
- linkage between raw mutation, realization, verification, and rollback
  records
- exact versus normalized-equivalent apply-shell outcomes
- forbidden Calc-local orchestration shortcuts that would reclaim apply-shell
  authority

Required artifact:

- one checked-in live apply-shell schema note

### 3. Build The Live Apply-Shell Observation And Classification Path

Make remaining live apply-shell gaps explicit enough to distinguish:

- exact engine-authored apply sequencing
- ordering-only host execution differences
- hidden host apply orchestration or repair
- missing realized or rolled-back live objects caused by apply-shell drift
- true queue, computational, graph, or replay divergence

This workstream should:

- add bounded admitted-slice apply-shell proof lanes
- surface apply-shell differences explicitly in test output
- keep the observation layer scoped only to the admitted slice

Required artifact:

- one checked-in live apply-shell observation and classification note

### 4. Build The Engine-Authored Live Apply Plan

Implement the narrowest change set needed to make admitted live apply
sequencing more explicitly engine-authored on the bounded slice.

This workstream should:

- add a stable engine-authored admitted apply-plan record or equivalent
  admitted sequencing instructions
- teach Calc to consume that apply plan without reclaiming hidden
  orchestration authority
- preserve engine-owned resident storage, resident wiring, lifetime, raw
  mutation, realization, and rollback decisions
- keep the underlying raw mutation APIs in Calc for this cycle

Required artifact:

- one checked-in implementation note for the engine-authored live apply plan

### 5. Freeze Differential Live Apply-Shell Evidence

Run the bounded live apply-shell proof cycle and record the results.

This evidence note should summarize:

- exact admitted-slice comparisons between engine-authored apply sequencing
  and the live Calc document after apply
- whether resident cell state, resident wiring state, formula-cell lifetime,
  realization, rollback, and final verification all continue to close
  exactly
- repair-detected and reject cases
- memory and performance observations
- whether the host-owned live apply shell actually shrinks in a meaningful
  way

Required artifact:

- one checked-in live apply-shell evidence note

### 6. Freeze The Live Apply-Shell Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-authored admitted-slice live apply-shell migration
- keep the live apply shell hybrid on the admitted slice
- defer broader live apply-shell migration again

The decision record must also state the next adjacent concern after this
closeout:

- broader admitted-slice host-shell reduction
- raw document mutation API migration
- or another newly bounded live apply-shell gap if this proof does not hold

Required artifact:

- one checked-in live apply-shell decision record

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ComputationalSubstrateRawMutation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx)
- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)
- [ComputationalSubstrateRollback.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx)
- [ComputationalSubstrateFormulaCellLifetime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx)
- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Recommended Execution Order

Run the workstreams in this order:

1. freeze the admitted live apply-shell contract
2. freeze the apply-plan schema
3. freeze the observation and classification model
4. implement the narrow engine-authored apply-plan path
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
- the admitted apply-shell proof lanes distinguish exact, rollback, and
  deferred outcomes explicitly
- the closeout says clearly whether the admitted live apply shell:
  - proceeds
  - remains hybrid
  - or is deferred again
- the next adjacent concern is stated explicitly
- all standing validation lanes remain green
- the zero-fallback replay baseline remains exact
