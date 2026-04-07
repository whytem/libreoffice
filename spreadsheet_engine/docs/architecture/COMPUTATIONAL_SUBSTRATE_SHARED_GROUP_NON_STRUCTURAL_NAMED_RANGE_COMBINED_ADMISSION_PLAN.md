# Computational Substrate Shared-Group Non-Structural Named-Range-Combined Admission Plan

Status: staged execution plan for the bounded named-range-combined cycle

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MULTI_GROUP_COLLAPSE_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MULTI_GROUP_COLLAPSE_DECISION_RECORD.md).

The merge-adjacent shared-group frontier is now materially narrower:

- exact same-sheet shareable member-exit scalar, formula, and clear are
  admitted
- exact same-sheet shareable same-text preserve is admitted
- exact same-sheet shareable edge-regroup is admitted
- exact same-sheet shareable gap-closing merge is admitted
- exact same-sheet shareable edge replacement-merge is admitted
- exact same-sheet shareable one-sided adjacent insertion is admitted
- bounded three-participant multi-group collapse is now explicitly closed as
  deferred because live Calc keeps the far group separate

That leaves the next logical frontier item:

- named-range-combined shared-group behavior

This plan therefore isolates named-range-combined promotion as its own proof
cycle instead of mixing it with repair-sensitive normalization, off-sheet
authority expansion, or broader non-edge regroup and merge classes.

## Plan Goal

Determine whether one bounded same-sheet shareable shared-group
named-range-combined family can move into the admitted slice with exact
authority.

The closeout must answer one explicit question:

- can the engine author one exact shared-group non-structural after-state
  while also closing exact named-range descriptor, dependency snapshot,
  broadcaster, graph, IR, realization, rollback, and final verification
  state

This plan succeeds even if the final result is narrower than broad
named-range-combined admission. It is acceptable to admit only one bounded
family, keep others validation-only, or defer them explicitly.

## Entry Boundary

This plan begins from the currently settled shared-group surface:

- structural shared-group `Preserve`, `Split`, and `Rebuild` are admitted
- non-structural shared-group member-exit scalar, formula, and clear are
  admitted
- non-structural shared-group same-text preserve is admitted
- non-structural shared-group edge-regroup is admitted
- non-structural shared-group gap merge, replacement merge, and one-sided
  adjacent insertion are admitted
- bounded three-participant multi-group collapse is explicitly deferred
- repair-sensitive normalization, off-sheet shared-group behavior, and
  broader non-edge regroup or merge remain deferred

This plan also begins from the already-separated named-range evidence:

- the bounded global single-area named-range class already exists as its own
  checked-in proof surface
- that class did not join the live structural rollout
- sheet-local, multi-area, and scope-ambiguous named-range classes remain
  deferred

The current shared-group non-structural authority path still treats
named-range drift as out of contract in
[AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx).

This cycle must therefore replace that broad defer rule with one bounded,
engine-authored named-range-combined candidate family instead of weakening
the exact-verification standard.

## Candidate Named-Range-Combined Slice

The first promotion attempt must stay narrow.

The primary candidate slice is:

- same-sheet shareable shared groups only
- non-structural `SetFormula` only
- the touched address is already shared before the mutation
- the touched address remains shared after the mutation
- same-text preserve only:
  - the formula source is unchanged
  - the shared-group topology is unchanged
- formulas reference only bounded global single-area named ranges
- named-range descriptors remain stable:
  - no add
  - no remove
  - no rename
  - no scope ambiguity
  - no multi-area targets
- named-range targets stay within the same-sheet authority surface already
  admitted for the shared-group slice
- no off-sheet consumers
- clean baseline only

Representative shape:

- an already-shared shareable formula like `=SUM(MyGlobalRange)` is replaced
  with identical source text on the edited sheet, Calc preserves the shared
  group, and the only new proof obligation is exact named-range-linked
  dependency and broadcaster closure

If this exact preserve family closes cleanly, the cycle may also decide
whether the same named-range authority model is strong enough to cover one
additional already-admitted topology family in the same pass:

- named-range-combined member-exit on the same bounded global single-area
  surface

That follow-on step is optional. The plan should not widen beyond it without
new checked-in scope.

## Non-Goals

This plan does not attempt to:

- admit sheet-local named-range-combined shared-group behavior
- admit multi-area or scope-ambiguous named ranges
- admit named-range-combined regroup
- admit named-range-combined merge
- admit named-range-combined multi-group collapse
- admit repair-sensitive host normalization
- admit off-sheet shared-group or off-sheet named-range consumers
- reopen the deferred global named-range structural rollout result
- claim broad named-range descriptor ownership in the engine

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in named-range-combined contract note
2. a checked-in named-range-combined scenario matrix
3. a checked-in named-range-combined mapping note
4. facade-side identification or classification proof for the bounded
   named-range-combined slice
5. a dedicated engine-authored named-range-combined candidate path
6. standalone and live exact proof or an explicit defer decision
7. a checked-in evidence note and decision record

## Workstreams

### 1. Freeze The Named-Range-Combined Contract

Define the exact admission candidate before changing runtime behavior.

This contract should freeze:

- which shared-group family is under consideration:
  - same-text preserve first
  - optional member-exit follow-on only if it closes on the same exact
    authority model
- which named-range class is under consideration:
  - global names only
  - single-area targets only
  - unambiguous descriptor resolution only
- which divergence patterns force immediate reject or defer:
  - named-range add, remove, or rename
  - sheet-local names
  - multi-area targets
  - scope ambiguity
  - off-sheet consumers
  - regroup, merge, or repair-sensitive behavior

### 2. Freeze The Named-Range-Combined Matrix

Define the exact workbook and mutation scenarios the proof cycle must cover.

This matrix should separate:

- same-text preserve with one global single-cell name
- same-text preserve with one global single-area range name
- formula text that preserves the same shared-group topology but changes
  named-range usage
- exact same-sheet name-target usage versus cases that spill into off-sheet
  consumers
- explicit reject buckets for local, multi-area, ambiguous, regroup,
  merge, and repair-sensitive cases

Each scenario should be marked as one of:

- candidate for admission
- validation-only evidence
- explicit reject or defer

### 3. Freeze The Mapping Rules

Define how shared-group and named-range identity are compared together.

This mapping note should state:

- how shared-group topology stays exact on the preserve surface
- how named-range descriptor identity is normalized for comparison
- how formula-to-name and name-to-target relationships are represented in
  the comparison surface
- what counts as exact versus normalized-equivalent on this bounded family
- which host-local named-range identities remain forbidden in comparisons

### 4. Land Facade-Side Named-Range-Combined Classification

Freeze a stable facade surface so tests can identify the bounded
named-range-combined family directly.

This workstream should:

- distinguish plain shared-group preserve from named-range-combined preserve
- distinguish bounded global single-area usage from local, multi-area, and
  ambiguous name use
- prove the candidate slice can be recognized without borrowing the live
  after-state as the source of truth

### 5. Land Engine-Authored Named-Range-Combined Prediction

Teach the shared-group non-structural candidate path to author one bounded
named-range-combined after-state instead of rejecting it generically.

This workstream should:

- preserve the current admitted surface by default
- replace the broad `shared_group_named_range_out_of_contract` rejection for
  the bounded candidate slice only
- keep exact shared-group topology closure mandatory
- keep exact named-range descriptor, dependency snapshot, broadcaster,
  graph, IR, realization, rollback, and final verification closure
  mandatory
- keep regroup, merge, repair-sensitive, and off-sheet named-range cases
  outside the live path

### 6. Prove Exact Closure

Run the bounded proof cycle and record whether the candidate closes exactly.

This proof must include:

- standalone computational proof
- shared-group topology comparison proof
- named-range dependency and broadcaster proof
- live lifecycle proof
- live mutation-entry proof
- retained reject coverage for broader named-range-combined frontier classes

### 7. Close Out The Cycle

Write the final evidence and decision record.

The closeout must state explicitly:

- whether the bounded named-range-combined preserve family is admitted
- whether optional member-exit follow-on was admitted or deferred
- which named-range-combined classes remain validation-only or deferred
- what the next adjacent concern becomes after this cycle

## Recommended Execution Order

1. Freeze the named-range-combined contract.
2. Freeze the named-range-combined matrix.
3. Freeze the named-range-combined mapping rules.
4. Land facade-side identification for the bounded preserve slice.
5. Land engine-authored named-range-combined preserve prediction.
6. Prove exact standalone and live closure.
7. Decide whether member-exit can ride the same exact authority model.
8. Write the evidence note and decision record.

## Success Criteria

This plan is successful when the repository contains a checked-in result
that makes the boundary clearer than it is today.

That means one of the following must be true:

- the bounded named-range-combined preserve family is admitted with exact
  proof
- the bounded family remains deferred with explicit live or standalone
  evidence showing why

This plan fails if it widens the shared-group path by inference instead of by
exact named-range-combined proof.
