# Computational Substrate Named-Range Structural Widening Plan

Status: complete closeout record

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_DECISION_RECORD.md).

The admitted rollout is now a bounded opt-in authority slice for:

- scalar lifecycle authority
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`
- ordinary scalar formulas only
- no shared groups
- no named-range-sensitive structural behavior

The next logical adjacent concern is therefore not broader storage migration
or shared-group rollout. It is the still-deferred class where structural edits
interact with formulas through named ranges.

This plan is the implementation-ready path for deciding whether a bounded
named-range-sensitive structural slice can be admitted into the same exact-
verification, exact-rollback rollout model.

This plan is now complete.

Its closeout result is:

- do not widen the live rollout
- keep the bounded global single-area class as validation-only
- keep sheet-local and multi-area named-range classes deferred

## Plan Goal

Establish whether `spreadsheet_engine` can safely drive a bounded named-range-
sensitive structural authority slice adjacent to the current admitted narrow
rollout.

The goal is not "named ranges in general." The goal is to determine whether a
carefully defined subset of structural edits whose observable behavior depends
on named ranges can be:

- modeled in the engine shadow and graph surfaces
- validated exactly against Calc
- rolled back immediately when the proof surface diverges
- promoted into the opt-in rollout only if the evidence is as strong as the
  currently admitted structural slice

## Entry Boundary

This plan begins from the current settled state:

- the first-stage extraction boundary is complete and stable
- the computational-substrate architecture program closed with a narrow-proceed
  result
- the narrow rollout is already wired and opt-in
- row/column insert-delete authority is admitted only for the ordinary scalar
  no-named-range slice
- named-range-sensitive structural behavior remains explicitly deferred in
  [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)

This plan must treat named-range-sensitive structural behavior as a new proof
surface, not as an already-admitted extension of the current rollout.

## Non-Goals

This plan should not attempt to:

- widen directly into shared-group-sensitive structural rollout
- admit broad named-range mutation authority such as unrestricted create,
  delete, rename, or scope-transfer behavior
- widen into sheet insert, delete, rename, or move
- widen into copy, move, clipboard, import, load-time, or undo-like flows
- move document storage, token-container ownership, or listener/broadcaster
  ownership into the engine
- weaken exact queue, graph, computational, and rollback standards

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note for the exact named-range-sensitive structural
   slice under evaluation
2. a checked-in workbook and mutation matrix for representative named-range
   structural cases
3. a checked-in mapping note defining named-range structural equivalence rules
   across Calc and engine shadows
4. a validation-only named-range structural pilot wired into the existing
   authority and differential lanes
5. a checked-in evidence note recording exact-match, normalized, rejected,
   rolled-back, and deferred outcomes
6. a checked-in decision record that either:
   - admits a bounded named-range-sensitive structural slice into the opt-in
     rollout
   - keeps the slice validation-only
   - or defers the slice again with explicit reasons

## Workstreams

### 1. Freeze The Named-Range Structural Widening Contract

Define the exact candidate slice before implementation begins.

This contract should freeze:

- which structural edits are in scope:
  - `InsertRows`
  - `DeleteRows`
  - `InsertColumns`
  - `DeleteColumns`
- which named-range classes are in scope:
  - global names versus sheet-local names
  - single-area versus multi-area names
  - names referenced by formulas on the edited sheet versus off-sheet
- which formula classes are in scope:
  - ordinary scalar formulas only at entry
- which divergence patterns force immediate defer:
  - shared-group repair
  - scope ambiguity
  - non-local structural repair not captured by the admitted exact-verification
    model

Required artifact:

- one checked-in named-range structural widening contract note

### 2. Freeze The Named-Range Workbook And Mutation Matrix

Define the exact scenario matrix that the proof cycle must cover.

This matrix should classify:

- formulas that reference named single-cell names
- formulas that reference named areas
- formulas that depend on names whose target area is widened or narrowed by a
  structural edit
- cross-sheet formulas that depend on named ranges on another sheet
- sheet-local names whose scope matches or differs from the edited sheet
- structural edits that cross, start inside, or sit adjacent to the named
  range target

Each scenario should be marked as one of:

- candidate for live promotion
- validation-only proof surface
- explicit defer

Required artifact:

- one checked-in named-range structural workbook and mutation matrix

### 3. Freeze The Named-Range Structural Mapping Rules

Define how Calc and engine-owned shadows are compared for named-range-sensitive
structural behavior.

This mapping note should state:

- how named-range descriptor identity is normalized
- how post-edit target area equivalence is compared
- how formula-to-name and name-to-target relationships are represented in the
  comparison surface
- which Calc-local identities are forbidden in comparisons
- when a result is exact versus normalized-equivalent versus rejected

Required artifact:

- one checked-in named-range structural mapping rules note

### 4. Build The Validation-Only Named-Range Structural Pilot

Extend the existing computational-substrate structural pilot so named-range-
sensitive structural cases can run through the same authority harness in
validation-only mode.

This workstream should:

- extend the shadow builders and mutation surfaces for named-range-sensitive
  structural rebuilds
- preserve Calc as the live owner unless a case is explicitly promoted later
- record queue, graph, computational, and rollback verdicts through the
  existing authority bridge
- keep promotion disabled until the evidence workstream closes

Required artifact:

- one checked-in implementation note for the validation-only named-range
  structural pilot

### 5. Freeze Named-Range Structural Evidence

Run the bounded proof cycle and record the results.

This evidence note should summarize:

- exact-match cases
- normalized-equivalent cases
- rollback-triggering cases
- repair-detected cases
- cases that remain validation-only or must be deferred
- whether global names and sheet-local names behave differently enough to
  split the promotion surface

Required artifact:

- one checked-in named-range structural evidence note

### 6. Freeze The Named-Range Rollout Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- admit a bounded named-range-sensitive structural slice into the opt-in
  rollout
- keep named-range-sensitive structural behavior validation-only
- defer it again because the proof surface is not yet stable enough

The decision record must also say whether the next adjacent concern after
named-range-sensitive structural behavior should be:

- shared-group-sensitive structural behavior
- sheet-level structural authority
- or another bounded reassessment before any widening

Required artifact:

- one checked-in named-range structural decision record

## Target Surfaces

The first implementation sweep should expect to touch:

- workbook facade named-range descriptor surfaces under
  `spreadsheet_engine/inc/spreadsheetengine/detail/workbook/`
- computational-substrate shadow and pilot surfaces under
  `spreadsheet_engine/inc/spreadsheetengine/detail/substrate/`
- Calc compat bridges under
  `spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/`
- Calc differential coverage under:
  - [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  - [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
  - [ucalc_compile_diff.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_compile_diff.cxx)
- standalone differential lanes under:
  - [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
  - [computational_graph_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_graph_tests.cxx)
  - [computational_execution_ir_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_execution_ir_tests.cxx)
- top-level docs:
  - [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
  - [README.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/README.md)

## Recommended Execution Order

Run this plan in the following order:

1. freeze the named-range structural widening contract
2. freeze the named-range workbook and mutation matrix
3. freeze the named-range structural mapping rules
4. build the validation-only named-range structural pilot
5. freeze named-range structural evidence
6. freeze the named-range rollout decision

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

If the validation-only named-range pilot introduces new exact-comparison or
rollback categories, those categories must be covered in checked-in Calc and
standalone tests before the plan can close.

## Exit Criteria

This plan is complete only when:

- the exact named-range-sensitive structural candidate slice is frozen
- the representative workbook and mutation classes are checked in
- comparison and normalization rules are documented and used by the pilot
- named-range-sensitive structural cases can run through the validation-only
  authority path with exact verdicts
- the final evidence clearly separates promotable, validation-only, and
  deferred classes
- the decision record says whether any named-range-sensitive structural subset
  is admitted into the opt-in rollout

If the proof does not support live widening, the plan still closes
successfully when it leaves behind:

- a bounded defer boundary
- a stable validation-only lane
- and a clear next adjacent concern
