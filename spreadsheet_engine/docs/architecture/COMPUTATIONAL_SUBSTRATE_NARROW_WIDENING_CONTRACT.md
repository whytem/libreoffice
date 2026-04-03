# Computational Substrate Narrow Widening Contract

Status: complete widening contract note for the narrow rollout plan

## Purpose

This note freezes the validation and promotion contract for the first bolder
near-adjacent widening candidates in the narrow rollout plan:

- `DeleteRows`
- `InsertColumns`

It exists so these candidates are judged against an explicit proof threshold
rather than promoted because they happen to look nearby to the already-admitted
`InsertRows` and `DeleteColumns` slice.

## Candidate Widening Surface

The widening candidates are:

- single-sheet `DeleteRows`
- single-sheet `InsertColumns`

They are not admitted by this contract. They are validation candidates only
until they separately satisfy the proof requirements below.

## Required Workbook Slice

Both candidates must be evaluated only on the same narrow workbook slice used
by the admitted structural rollout:

- ordinary scalar formulas only
- no shared groups
- no named-range-sensitive structural behavior
- no external-reference-sensitive structural behavior
- no copy, move, clipboard, load-time, or undo-like structural flows
- clean baseline only
- local reference updates only for formulas that remain ordinary single-cell
  formulas after the mutation

If a scenario leaves that slice, it is evidence for defer, not for partial
promotion.

## Promotion Threshold

`DeleteRows` or `InsertColumns` may be promoted only if each candidate proves
all of the following on its own:

- exact queue verification
- exact computational verification
- exact graph verification
- explicit reference-update expectations that match the live after-state
- deterministic rejection for dirty-baseline entry
- deterministic rejection when the mutation drifts outside the admitted slice
- deterministic rollback or repair-detected behavior when divergence is
  injected deliberately
- no hidden Calc repair required for the successful cases

No normalized-equivalent outcome is sufficient for promotion in this contract.

## Required Evidence Shape

For each candidate mutation class, the widening evidence must include:

- one standalone or substrate-lane validation case
- one Calc differential happy-path case
- one dirty-baseline rejection case
- one out-of-contract rejection or validation-only classification case
- one rollback or repair-detected divergence case

Each candidate is judged independently:

- `DeleteRows` may be promotable while `InsertColumns` remains deferred
- `InsertColumns` may be promotable while `DeleteRows` remains deferred

## Divergence Patterns That Force Defer

The following patterns force defer for the affected candidate:

- need for shared-group repair
- need for named-range-sensitive structural repair
- hidden Calc repair on the supposed happy path
- queue, computational, or graph mismatch after admitted application
- reference-update mismatch on the supposed happy path
- inability to keep rollback deterministic
- evidence that the candidate only works by widening beyond the admitted
  workbook slice

## What This Contract Allows

This contract allows the next workstream to answer three different outcomes
honestly for each candidate:

- promotable now
- validation-only, not yet promotable
- deferred

It does not allow "probably promotable" as a closeout answer.
