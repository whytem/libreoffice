# Computational Substrate Narrow Rollout Plan

Status: active implementation-ready plan

## Purpose

This document describes the next implementation plan after the completed
[COMPUTATIONAL_SUBSTRATE_PHASE7_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE7_DECISION_RECORD.md).

Phase 7 closed the computational-substrate architecture program with a narrow
proceed decision rather than a broad rollout or a stop. The next step is
therefore not another architecture reassessment. It is bounded rollout
engineering on the admitted authority slice.

This plan also incorporates the intentionally bolder execution step selected
after the Phase 7 closeout:

- do not stop at the already-admitted slice
- roll out that admitted slice behind explicit gates
- in parallel, aggressively validate the nearest adjacent structural classes
  (`DeleteRows` and `InsertColumns`) as the first promotion candidates

## Plan Goal

Implement a bounded experimental rollout for the admitted computational
substrate authority slice while generating the evidence needed to either:

- keep that rollout narrow and stable
- widen it one step to include `DeleteRows` and `InsertColumns`
- or narrow it again immediately if the rollout evidence regresses

## Entry Boundary

This plan begins from the Phase 7 narrow-proceed boundary:

- admitted scalar lifecycle authority
- admitted single-sheet `InsertRows`
- admitted single-sheet `DeleteColumns`
- ordinary-scalar-formula slice only
- no shared groups
- no named-range-sensitive structural behavior
- exact queue, computational, and graph verification
- repair-detected rollback on admitted structural divergence

The bolder execution step may validate `DeleteRows` and `InsertColumns`, but
it must not treat them as already admitted at entry.

## Non-Goals

This plan should not attempt to:

- broaden directly into shared-group or named-range-sensitive rollout
- move document-wide storage ownership into the engine
- migrate `ScTokenArray` ownership
- widen into sheet insert, delete, rename, or move
- widen into copy, move, clipboard, load-time, or undo-like structural flows
- weaken exact verification or rollback requirements for the admitted slice

## Required Deliverables

This plan is complete only when all of the following exist:

1. an explicit rollout contract and authority-gate note for the admitted slice
2. a Calc-side opt-in experimental rollout path for the admitted slice
3. a checked-in widening contract for `DeleteRows` and `InsertColumns`
4. checked-in evidence for whether those two classes are promotable
5. a checked-in rollout evidence summary after bounded experimental bake time
6. a checked-in decision record saying whether the rollout:
   - stays on the original admitted slice
   - widens to include the newly proven structural classes
   - narrows again

## Workstreams

### 1. Freeze The Narrow Rollout Contract

Freeze the exact rollout contract for the already-admitted slice before any
runtime enabling work begins.

The contract should name:

- the admitted mutation and workbook classes
- the required authority gates
- exact verification and rollback requirements
- what counts as rollout success versus immediate deactivation

Required artifact:

- one checked-in narrow-rollout contract note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_CONTRACT.md)

### 2. Wire The Opt-In Experimental Rollout Path

Implement the actual opt-in runtime path for the admitted authority slice.

This workstream should:

- make the admitted slice enableable without broad default-on behavior
- preserve exact verification and rollback
- keep the standing differential lanes authoritative
- make deactivation straightforward if a bounded rollout regresses

Required artifact:

- one checked-in implementation note describing the rollout gate and enabled
  surface

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_IMPLEMENTATION.md)

### 3. Freeze The Bolder Widening Contract

Define the explicit validation contract for the two bolder near-adjacent
promotion candidates:

- `DeleteRows`
- `InsertColumns`

This contract should state:

- the exact admitted workbook slice
- what counts as proof strong enough for promotion
- what divergence patterns force defer instead

Required artifact:

- one checked-in widening contract note for `DeleteRows` and `InsertColumns`

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md)

### 4. Build Widening Validation And Promotion Evidence

Run the bolder execution step in a controlled way.

This workstream should:

- add or freeze validation lanes for `DeleteRows` and `InsertColumns`
- collect exact-match, rollback, and repair-detected evidence
- determine whether each class is:
  - promotable
  - validation-only
  - deferred

Required artifact:

- one checked-in widening evidence note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_EVIDENCE.md)

### 5. Freeze Bounded Rollout Evidence

Capture the state of the admitted rollout after bounded experimental bake
time.

This workstream should summarize:

- correctness on the admitted rollout slice
- operational clarity and debugging cost
- performance and memory observations
- whether the bolder widening candidates strengthened or weakened the rollout
  case

Required artifact:

- one checked-in bounded rollout evidence note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_EVIDENCE.md)

### 6. Freeze The Rollout Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- keep the narrow rollout as-is
- widen it to include newly proven `DeleteRows` and/or `InsertColumns`
- narrow again because the experimental rollout did not hold up

Required artifact:

- one checked-in rollout decision record

## Bolder Execution Step

The intentional boldness in this plan is:

- rollout work does not wait for another large architecture phase
- `DeleteRows` and `InsertColumns` are treated as immediate promotion
  candidates, not distant future work
- promotion is allowed as soon as exact verification and rollback evidence are
  as strong as the already-admitted row-insert and column-delete slice

The plan stays disciplined by coupling that boldness with:

- explicit contract freeze
- exact verification
- rollback and repair-detected classification
- narrow workbook-slice constraints

## Target Surfaces

The first implementation sweep should expect to touch:

- computational substrate compat headers under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- substrate pilot builders under
  `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- Calc differential coverage under:
  - [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  - [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
  - [ucalc_compile_diff.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_compile_diff.cxx)
- standalone lanes under:
  - [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
  - [computational_graph_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_graph_tests.cxx)
  - [computational_execution_ir_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_execution_ir_tests.cxx)
- top-level docs:
  - [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
  - [README.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/README.md)

## Recommended Execution Order

Run this plan in the following order:

1. freeze the narrow rollout contract
2. wire the opt-in experimental rollout path
3. freeze the widening contract for `DeleteRows` and `InsertColumns`
4. build widening validation and promotion evidence
5. freeze bounded rollout evidence
6. freeze the rollout decision

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

Additional rollout-specific validation should include:

- explicit applied, rejected, rollback, and repair-detected cases for:
  - admitted scalar lifecycle slice
  - admitted `InsertRows`
  - admitted `DeleteColumns`
  - candidate `DeleteRows`
  - candidate `InsertColumns`
- at least one bounded end-to-end bake-time evidence summary

## Exit Criteria

This plan is complete only if:

1. the admitted slice is available behind an explicit opt-in rollout path
2. exact verification and rollback remain intact
3. `DeleteRows` and `InsertColumns` are classified honestly as promoted,
   validation-only, or deferred
4. rollout evidence is written down rather than inferred
5. the final decision says clearly whether the rollout stayed narrow, widened,
   or narrowed again

## Definition Of Success

This plan is a success if it turns the Phase 7 narrow-proceed decision into a
real bounded rollout path and uses the bolder widening step to answer the next
obvious promotion question quickly and honestly.

It is not a success if it adds more pilot machinery without either:

- enabling the admitted slice
- or producing a clear promotion/defer answer for `DeleteRows` and
  `InsertColumns`
