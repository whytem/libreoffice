# Computational Substrate Shared-Group Non-Structural Admission Plan

Status: complete closeout record for the non-structural shared-group admission cycle

## Closeout Result

This plan is now complete.

The bounded promotion result is:

- admit same-sheet shareable shared-group non-structural member-exit
  `SetScalarValue`, `SetFormula`, and `ClearCell` cases into the live
  authority and lifecycle slice behind a dedicated non-structural
  shared-group gate
- carry that same bounded slice through the mutation-entry path behind the
  existing explicit mutation-entry gate plus the dedicated non-structural
  shared-group gate
- keep regroup, merge, same-text preserve, named-range-combined,
  repair-sensitive, off-sheet, and broader workbook classes deferred

The closeout references are now:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_SCENARIO_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_SCENARIO_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MAPPING_RULES.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MAPPING_RULES.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_DECISION_RECORD.md)

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md).

The shared-group widening cycle closed with a deliberately bounded result:

- the live opt-in rollout widened only for exact same-sheet shareable
  structural shared-group `Preserve`, `Split`, and `Rebuild`
- non-exact shared-group behavior stayed on the validation-only lane
- non-structural shared-group split or rebuild behavior stayed deferred

That broad question is now answered. The next actionable question is
narrower:

- can a bounded same-sheet shareable non-structural shared-group slice meet
  the same exact-verification standard as the admitted structural family

This plan is therefore not another broad shared-group widening effort. It is
the implementation-ready path for deciding whether member-local
`SetScalarValue`, `SetFormula`, and `ClearCell` edits that reshape an
existing shared group can be admitted into the current rollout.

## Plan Goal

Determine whether `spreadsheet_engine` can safely promote a bounded
non-structural shared-group slice through the existing lifecycle and
authority path.

The closeout must answer one explicit question:

- admit a bounded non-structural shared-group slice into the live opt-in
  rollout
- keep it validation-only
- or defer it again with explicit reasons

## Entry Boundary

This plan begins from the current settled state:

- the first-stage extraction boundary is complete and stable
- the computational-substrate program already has an ownership-complete
  admitted slice plus one bounded shared-group structural widening
- the current live rollout already admits:
  - scalar mutation entry
  - scalar lifecycle authority
  - same-sheet structural `InsertRows`, `DeleteRows`, `InsertColumns`, and
    `DeleteColumns`
  - exact same-sheet shareable shared-group structural `Preserve`, `Split`,
    and `Rebuild`
- non-structural shared-group split or rebuild outcomes are still deferred in
  [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
- the lifecycle builder still admits only ordinary ungrouped formula cells in
  [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
- the authority helper still applies non-structural mutations cell-locally in
  [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
  without a shared-group-aware after-state rule family
- the rollout surfaces already expose a dedicated structural shared-group gate
  in [ComputationalSubstrateRollout.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx),
  but no separate non-structural shared-group promotion gate
- the workbook facade already exposes shared-group descriptor collection and
  transition classification in [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)

This plan must therefore treat non-structural shared-group behavior as a new
promotion candidate, not as an automatic extension of the structural slice.

## Candidate Slice

The promotion target must stay narrower than "shared-group lifecycle in
general."

The first candidate slice is:

- same-sheet shareable shared groups only
- clean baseline only
- one touched pre-existing group on the edited sheet
- non-structural member-local mutations only:
  - `SetScalarValue`
  - `SetFormula`
  - `ClearCell`
- exact after-state derivable from the pre-mutation group and the mutation
  without copying host-observed group topology

The narrowest first promotion family should be:

- destructive member-exit edits where the touched cell leaves the shared
  group and the surviving pre-existing group members are repartitioned into
  contiguous runs
- `SetScalarValue` on a shared-group anchor or member
- `ClearCell` on a shared-group anchor or member
- `SetFormula` on a shared-group anchor or member only when the replacement
  formula is explicitly treated as a non-grouped formula cell in the
  candidate rule set

That bounded family is enough to prove non-structural shared-group `Split`,
`Rebuild`, or collapse-to-`None` outcomes without first solving broader host
regroup heuristics.

## Non-Goals

This plan should not attempt to:

- admit broad regroup or merge behavior across prior shared groups
- infer new shared-group creation from arbitrary formula replacement text
- widen into named-range-combined shared-group behavior
- widen into off-sheet or broader workbook classes
- widen into sheet insert, delete, rename, or move
- widen into copy, move, clipboard, import, load-time, or undo-like flows
- reopen storage residency, token-container ownership, or live wiring
  residency decisions that are already settled on the admitted slice
- weaken exact queue, computational, graph, IR, realization, rollback, or
  verification requirements

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the bounded non-structural
   shared-group candidate slice
2. a checked-in mutation and scenario matrix covering representative anchor,
   member, split, rebuild, collapse, reject, and repair-sensitive outcomes
3. a checked-in mapping note defining non-structural shared-group identity,
   partitioning, and forbidden host shortcuts
4. a dedicated gated runtime path for the candidate slice through lifecycle
   and authority apply
5. a checked-in evidence note recording exact, rejected, repair-detected,
   rollback, and deferred outcomes
6. a checked-in decision record that either:
   - admits the bounded non-structural slice into the live rollout
   - keeps it validation-only
   - or defers it again with explicit reasons

## Workstreams

### 1. Freeze The Non-Structural Shared-Group Admission Contract

Define the exact candidate slice before implementation begins.

This contract should freeze:

- which mutation kinds are in scope:
  - `SetScalarValue`
  - `SetFormula`
  - `ClearCell`
- which touched cells are in scope:
  - shared-group anchor
  - shared-group interior member
  - shared-group tail member
- which outcomes are candidate versus deferred:
  - exact member-exit split
  - exact member-exit rebuild
  - exact member-exit collapse to no surviving shared group
  - regroup or merge behavior stays deferred
- which workbook classes stay out of scope:
  - named-range-combined
  - off-sheet
  - non-shareable
  - repair-sensitive host-only regrouping
  - dirty baseline

Required artifact:

- one checked-in non-structural shared-group admission contract note

### 2. Freeze The Non-Structural Scenario Matrix

Define the exact proof surface that the promotion cycle must cover.

This matrix should classify:

- `SetScalarValue` on anchor, interior member, and tail member
- `ClearCell` on anchor, interior member, and tail member
- `SetFormula` on anchor, interior member, and tail member
- cases that produce:
  - `Split`
  - `Rebuild`
  - `None`
- cases that must remain reject, repair-detected, validation-only, or
  deferred

The matrix should explicitly separate:

- destructive member-exit cases that are candidates for live promotion
- formula replacements whose exact shareability or regrouping semantics are
  not yet engine-authored
- multi-group, merge, or named-range-combined cases that stay deferred

Required artifact:

- one checked-in non-structural shared-group scenario matrix

### 3. Freeze The Non-Structural Mapping Rules

Define how the candidate slice predicts after-state identity without relying
on host-observed topology.

This mapping note should state:

- stable before-group identity for the touched group
- stable identity for the touched cell role:
  - anchor
  - member
  - tail
- how the candidate rule removes or replaces the touched cell in the
  predicted after-state
- how surviving pre-existing group members are repartitioned into contiguous
  runs
- when a surviving run remains a shared group versus becoming an ordinary
  formula cell
- when the edited cell is represented as scalar, empty, or a non-grouped
  formula cell
- which Calc-local regroup or normalization shortcuts remain forbidden

Required artifact:

- one checked-in non-structural shared-group mapping rules note

### 4. Build The Bounded Non-Structural Candidate Path

Extend the current lifecycle and authority path so the candidate slice can
run through exact engine-authored after-state prediction.

This workstream should:

- extend facade-side shared-group helpers to identify the touched group and
  summarize the resulting non-structural transition
- replace the current `formula_shape_out_of_contract` rejection for the
  bounded candidate slice in
  [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
- add a shared-group-aware non-structural predictor in
  [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
  and the lifecycle bridge
- author after-state shared-group topology by repartitioning surviving
  members rather than copying observed-after topology
- carry the authored after-state through computational, graph, IR,
  formula-cell lifetime, cell storage, and wiring realization
- add a dedicated non-structural shared-group rollout sub-gate in
  [ComputationalSubstrateRollout.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx)
  and thread it through the compat apply surfaces
- keep the existing structural shared-group lane unchanged
- keep out-of-scope cases rejected, validation-only, or repair-detected

Required artifact:

- one checked-in implementation note for the non-structural shared-group
  candidate path

### 5. Freeze Non-Structural Shared-Group Evidence

Run the bounded proof cycle and record the results.

This evidence note should summarize:

- facade classification coverage for representative non-structural split,
  rebuild, and collapse outcomes
- standalone proof that the candidate predictor does not depend on
  host-observed after-state topology
- exact live apply for the gated candidate slice through queue,
  computational, graph, and IR verification
- exact formula-cell lifetime, cell-storage, and wiring realization on the
  admitted candidate slice
- repair-detected or rollback behavior for perturbed after-states
- deterministic reject behavior when the candidate gate is off
- replay-baseline impact, including confirmation that the zero-fallback
  baseline remains intact

Required artifact:

- one checked-in non-structural shared-group evidence note

### 6. Freeze The Rollout Decision

Close the plan with an explicit decision record.

The closeout must say one of:

- admit the bounded non-structural shared-group candidate slice into the
  live rollout
- keep it validation-only because exact engine-authored prediction still does
  not close
- defer it again because the remaining behavior still depends on host-only
  regroup or repair

The decision record must also name the next adjacent concern after this
cycle:

- broader shared-group regroup or merge behavior
- named-range-combined shared-group behavior
- or another bounded reassessment before any further widening

Required artifact:

- one checked-in non-structural shared-group decision record

## Planned Execution Order

The execution order should stay intentionally staged:

1. land the contract, matrix, and mapping rules before changing runtime
   behavior
2. implement the destructive member-exit candidate family first
3. prove exactness without host-observed topology on standalone tests
4. thread the exact candidate through live lifecycle and authority apply
5. only then reassess whether any shareable formula-replacement rebuild cases
   can join the same slice without widening into host regroup heuristics

This staging avoids mixing the first credible promotion candidate with the
harder regroup and merge boundary.

## Target Surfaces

The first implementation sweep should expect to touch:

- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)
- [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
- [ComputationalSubstrateLifecycle.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLifecycle.hxx)
- [ComputationalSubstrateAuthority.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ComputationalSubstrateRollout.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx)
- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- [ucalc_workbook_facade.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_workbook_facade.cxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Success Criteria

This plan is successful only if all of the following become true on one
explicitly bounded candidate family:

- the engine predicts after-mutation shared-group topology without copying
  host-observed after-state topology
- queue, computational, graph, IR, lifetime, storage, and wiring checks all
  close to the same standard already required on the admitted slice
- rollback and repair-detected behavior remain explicit for out-of-scope or
  perturbed cases
- the rollout boundary stays narrow and documentable
- the closeout clearly states which non-structural shared-group classes are
  now admitted and which remain deferred
