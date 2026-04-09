# Computational Substrate Global Named-Range Admission Plan

Status: complete closeout record

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_DECISION_RECORD.md).

The named-range widening plan closed with a deliberately narrow result:

- the live opt-in rollout did not widen
- the bounded global single-area named-range class stayed validation-only
- sheet-local, multi-area, and scope-ambiguous classes stayed deferred

That closeout answered the broad named-range question. The next actionable
question is narrower:

- can the already-proven global single-area validation-only slice satisfy the
  same live admission standard as the current structural rollout surface

This plan is therefore not another broad named-range widening effort. It is a
promotion proof cycle for one already-separated candidate slice.

This plan is now complete.

Its closeout result is:

- do not widen the live opt-in rollout
- keep the bounded global single-area slice out of the live rollout
- keep off-sheet, local, multi-area, and scope-ambiguous classes deferred

## Plan Goal

Determine whether `spreadsheet_engine` can safely promote the bounded global
single-area named-range structural slice from validation-only status into the
same opt-in exact-verification rollout model already used for the admitted
single-sheet scalar structural surface.

The goal is to decide one explicit question:

- admit the bounded global named-range slice into the live opt-in rollout
- keep it validation-only
- or defer it again with explicit reasons

## Entry Boundary

This plan begins from the current settled state:

- the first-stage extraction boundary is complete and stable
- the computational-substrate architecture program closed with a narrow
  authority result
- the opt-in narrow rollout is complete and stable for:
  - scalar lifecycle authority
  - single-sheet `InsertRows`
  - single-sheet `DeleteRows`
  - single-sheet `InsertColumns`
  - single-sheet `DeleteColumns`
  - ordinary scalar formulas only
  - no shared groups
  - no named-range-sensitive structural behavior
- the global single-area named-range class already exists as a
  validation-only structural pilot surface
- sheet-local, multi-area, and scope-ambiguous named-range classes remain
  deferred

This plan must treat the global single-area class as a promotion candidate,
not as an already-admitted rollout surface.

## Non-Goals

This plan should not attempt to:

- widen directly into sheet-local named-range structural behavior
- widen directly into multi-area or scope-ambiguous named-range classes
- widen directly into shared-group-sensitive structural behavior
- widen into sheet insert, delete, rename, or move
- widen into copy, move, clipboard, import, load-time, or undo-like flows
- claim broad named-range descriptor ownership in the engine
- move storage, token-container ownership, or listener/broadcaster ownership
  into the engine
- weaken exact queue, computational, graph, and rollback standards

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the exact global named-range admission
   candidate
2. a checked-in workbook and mutation matrix for the promotion proof surface
3. a checked-in mapping and equivalence note defining exact-match versus
   normalized-equivalent named-range outcomes
4. a gated live-candidate path that can run the bounded global named-range
   slice through the existing authority bridge without broadening the rest of
   the rollout
5. a checked-in evidence note recording exact, normalized, rejected,
   rollback, and repair-detected outcomes for the candidate slice
6. a checked-in decision record that either:
   - admits the bounded global single-area slice into the opt-in rollout
   - keeps it validation-only
   - or defers it again with explicit reasons

## Workstreams

### 1. Freeze The Global Named-Range Admission Contract

Define the exact candidate slice before any live-candidate wiring begins.

This contract should freeze:

- which named-range class is under consideration:
  - global names only
  - single-area targets only
  - unambiguous descriptor resolution only
- which structural edits are in scope:
  - `InsertRows`
  - `DeleteRows`
  - `InsertColumns`
  - `DeleteColumns`
- which workbook and formula classes are in scope:
  - ordinary scalar formulas only
  - same-sheet structural edits only
  - clean baseline only
- which divergence patterns force immediate rejection or defer:
  - dirty baseline
  - repair-detected structural divergence
  - scope ambiguity
  - multi-area targets
  - sheet-local names
  - shared-group interaction

Required artifact:

- one checked-in global named-range admission contract note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_CONTRACT.md)

### 2. Freeze The Global Named-Range Workbook And Mutation Matrix

Define the exact scenario matrix that the promotion proof cycle must cover.

This matrix should classify:

- formulas that reference a global named single cell
- formulas that reference a global named single-area range
- formulas on the edited sheet versus off-sheet formulas that reference the
  same global name
- structural edits that start before, inside, after, or exactly on the named
  target boundary
- cases where the source text preserves an explicit sheet prefix versus cases
  that rely on workbook-global identity alone

Each scenario should be marked as one of:

- candidate for live admission
- validation-only evidence
- explicit reject or defer

Required artifact:

- one checked-in global named-range workbook and mutation matrix

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_MATRIX.md)

### 3. Freeze The Global Named-Range Equivalence Rules

Define how Calc and engine-owned surfaces are compared for this promotion
question.

This mapping note should state:

- how global named-range descriptor identity is normalized
- how single-area target equivalence is compared after structural edits
- how formula-to-name and name-to-target relationships are represented in the
  comparison surface
- which after-state differences still count as exact versus
  normalized-equivalent
- which Calc-local identities remain forbidden in comparisons

Required artifact:

- one checked-in global named-range equivalence rules note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_EQUIVALENCE_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_EQUIVALENCE_RULES.md)

### 4. Build The Global Named-Range Live-Candidate Path

Extend the existing structural authority harness so the bounded global
single-area named-range slice can run as a separately gated live-candidate
path.

This workstream should:

- preserve the current rollout surface by default
- add a distinct opt-in gate or sub-gate for the bounded global named-range
  slice
- keep exact queue, computational, and graph verification mandatory
- keep repair-detected rollback mandatory
- keep non-global, multi-area, local, or ambiguous cases out of the live path

Required artifact:

- one checked-in implementation note for the global named-range
  live-candidate path

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_IMPLEMENTATION.md)

### 5. Freeze Global Named-Range Admission Evidence

Run the bounded promotion proof cycle and record the results.

This evidence note should summarize:

- exact-match cases
- normalized-equivalent cases, if any are needed
- rollback-triggering cases
- repair-detected cases
- deterministic reject cases
- whether off-sheet formula consumers of global names behave homogeneously
  enough to stay inside one promotion surface

Required artifact:

- one checked-in global named-range admission evidence note

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_EVIDENCE.md)

### 6. Freeze The Global Named-Range Admission Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- admit the bounded global single-area named-range slice into the opt-in
  rollout
- keep it validation-only
- defer it again because the promotion surface is still not stable enough

The decision record must also state the next adjacent concern after this
promotion question:

- sheet-local named-range structural behavior
- shared-group-sensitive structural behavior
- or another bounded reassessment before any widening

Required artifact:

- one checked-in global named-range admission decision record

The checked-in artifact for this workstream is:

- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_DECISION_RECORD.md)

## Target Surfaces

The first implementation sweep should expect to touch:

- computational-substrate pilot builders under
  `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- rollout and authority compat headers under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
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

1. freeze the global named-range admission contract
2. freeze the workbook and mutation matrix
3. freeze the equivalence rules
4. build the live-candidate path
5. freeze the admission evidence
6. freeze the admission decision

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

If the live-candidate path introduces a new gate or new verdict categories,
those must be covered in checked-in Calc and standalone tests before the plan
can close.

## Exit Criteria

This plan is complete only when:

- the exact global named-range promotion candidate is frozen
- the representative workbook and mutation matrix is checked in
- equivalence rules are documented and used by the proof lanes
- the bounded global single-area slice can run through a separately gated
  live-candidate path with exact verdicts
- the evidence clearly separates promotable, validation-only, and deferred
  classes
- the decision record says whether the global single-area slice joins the
  opt-in rollout

If the proof does not support live admission, the plan still closes
successfully when it leaves behind:

- a stable validation-only lane for the global slice
- a clear reason the promotion standard was not met
- and an explicit next adjacent concern
