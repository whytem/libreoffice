# Computational Substrate Final Verification Host-Shell Reassessment Plan

Status: completed closeout record

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_DECISION_RECORD.md).

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

What still remains host-owned on that same slice is now narrower:

- the final verification host shell around admitted primitive realization
  and rollback execution
- any workbook or mutation class outside the admitted slice

This plan is not a broad `ScDocument` transplant. It is the next bounded
reassessment of whether the retained final verification shell can become
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
- primitive realization and primitive rollback records

That means the most actionable remaining host-owned choke point on the
bounded slice is no longer storage, wiring, lifetime, mutation identity,
live apply sequencing, or primitive realization and rollback identity. It is
the final verification shell that still decides whether admitted
engine-authored state is accepted as exact.

The shortest path toward broader engine-owned live authority is therefore:

1. define one explicit engine-authored final verification record or plan for
   the admitted slice
2. keep Calc as the temporary primitive execution host
3. preserve exact queue, computational, graph, replay, realization, and
   rollback proof
4. decide only after that proof whether broader host-shell reduction is
   justified

## Plan Goal

Determine whether `spreadsheet_engine` can safely own more of the admitted
final verification host shell on the admitted slice, so that Calc no longer
quietly decides admitted exactness through hidden local host sequencing after
the engine-authored mutation, realization, rollback, and primitive
realization/rollback records have already been built.

The goal is to decide one explicit question:

- proceed with engine-authored admitted-slice final verification migration
- keep the final verification shell hybrid on the admitted slice
- or defer broader final verification migration again

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
- exact queue, computational, graph, replay, realization, and rollback
  verification already holds on the admitted slice
- Calc still owns:
  - the final verification host shell around admitted primitive realization
    and rollback execution

This plan must therefore treat final verification as the next proof surface,
not as an already-admitted extension of the current boundary.

## Non-Goals

This plan should not attempt to:

- move UI, UNO, rendering, import/export, persistence, or environment
  services into the engine
- replace all `ScDocument` verification behavior in one sweep
- widen beyond the admitted scalar lifecycle and admitted narrow structural
  slice
- widen into shared-group-sensitive or named-range-sensitive behavior
- widen into sheet insert, delete, rename, or move
- migrate token-container ownership
- reopen resident storage, resident wiring, lifetime, raw mutation, raw
  document mutation, live apply, or primitive realization/rollback
  migration
- weaken exact queue, computational, graph, replay, realization, or
  rollback requirements
- claim broad `ScDocument` host independence

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the admitted final verification
   surface
2. a checked-in schema note for the engine-authored final verification
   record or plan and verdicts
3. a checked-in observation and classification note for final verification
   drift
4. a checked-in implementation note for the engine-authored final
   verification path
5. a checked-in evidence note covering exact, reject, rollback, and deferred
   final verification outcomes
6. a checked-in decision record saying whether admitted-slice final
   verification migration:
   - proceeds
   - remains hybrid
   - or is deferred again

## Workstreams

### 1. Freeze The Final Verification Contract

Freeze the exact admitted final verification surface before implementation
work begins.

This contract should name:

- the admitted workbook and mutation classes
- the exact final verification classes under reassessment:
  - admitted verification of resident cell state after apply
  - admitted verification of resident wiring state after apply
  - admitted verification of formula-cell lifetime realization after apply
  - admitted verification of primitive rollback restore after reject
  - admitted runtime combinations of raw mutation, raw document mutation,
    live apply, primitive realization/rollback, and final verification
- retained Calc-owned host surfaces that stay out of scope in this cycle
- exact success criteria versus immediate hybrid or defer triggers

Required artifact:

- one checked-in final verification contract note

### 2. Freeze The Final Verification Schema

Define the representative admitted-slice final verification shape that the
proof cycle must use.

This schema note should define:

- engine-authored admitted final verification records or plans
- stable verification identity, normalized payload rules, and verdict
  categories
- linkage between final verification and the admitted live apply and
  primitive realization/rollback records
- exact versus normalized-equivalent final verification outcomes
- forbidden Calc-local shortcuts that would reclaim verification authority

Required artifact:

- one checked-in final verification schema note

### 3. Build The Final Verification Observation Path

Make remaining final verification gaps explicit enough to distinguish:

- exact engine-authored final verification
- ordering-only host verification differences
- hidden host verification orchestration or repair
- missing verification inputs caused by primitive shell drift
- true queue, computational, graph, replay, realization, or rollback
  divergence

This workstream should:

- add bounded admitted-slice final verification proof lanes
- surface final verification differences explicitly in test output
- keep the observation layer scoped only to the admitted slice

Required artifact:

- one checked-in final verification observation and classification note

### 4. Build The Engine-Authored Final Verification Path

Implement the narrowest change set needed to make admitted final
verification more explicitly engine-authored on the bounded slice.

This workstream should:

- add stable engine-authored admitted final verification records or
  equivalent instructions
- teach Calc to consume those verification surfaces without reclaiming
  hidden verification authority
- preserve engine-owned resident storage, resident wiring, lifetime, raw
  mutation, raw document mutation, live apply, and primitive
  realization/rollback decisions
- keep primitive execution host operations in Calc for this cycle

Required artifact:

- one checked-in implementation note for the engine-authored final
  verification path

### 5. Freeze Differential Final Verification Evidence

Run the bounded final verification proof cycle and record the results.

This evidence note should summarize:

- exact admitted-slice comparisons between engine-authored final
  verification and the live Calc document after apply or reject
- whether resident cell state, resident wiring state, formula-cell lifetime,
  raw mutation, raw document mutation, realization, rollback, live apply,
  and primitive realization/rollback all continue to close exactly
- reject, rollback, and deferred cases
- memory and performance observations
- whether the host-owned final verification shell actually shrinks in a
  meaningful way

Required artifact:

- one checked-in final verification evidence note

### 6. Freeze The Final Verification Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- proceed with engine-authored admitted-slice final verification migration
- keep the final verification shell hybrid on the admitted slice
- defer broader final verification migration again

The decision record must also state the next adjacent concern after this
closeout:

- another newly bounded verification-host gap
- another newly bounded admitted-slice live host shell
- or another newly bounded adjacent concern if this proof does not hold

Required artifact:

- one checked-in final verification decision record

## Target Surfaces

The most likely implementation surfaces for this plan are:

- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ComputationalSubstrateLiveApply.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLiveApply.hxx)
- [ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx)
- [ComputationalSubstrateRollback.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx)
- [ComputationalSubstrateRawMutation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Recommended Execution Order

Run the workstreams in this order:

1. freeze the admitted final verification contract
2. freeze the final verification schema
3. freeze the observation and classification model
4. implement the narrow engine-authored final verification path
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
- the admitted final verification proof lanes distinguish exact, reject,
  rollback, and deferred outcomes explicitly
- the closeout says clearly whether the admitted final verification shell:
  - proceeds
  - remains hybrid
  - or is deferred again
- the next adjacent concern is stated explicitly
- all standing validation lanes remain green
- the zero-fallback replay baseline remains exact
