# Computational Substrate Named-Range Structural Contract

Status: complete contract note for the named-range structural widening plan

## Purpose

This note freezes the exact named-range-sensitive structural slice that the
next proof cycle is allowed to evaluate.

It exists so named-range-sensitive structural behavior is judged against an
explicit boundary instead of drifting outward during implementation.

## Mutation Classes In Scope

The only structural mutation classes in scope are:

- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`

The mutation must still satisfy the existing narrow-rollout entry rules:

- clean baseline only
- ordinary scalar formulas only
- no shared groups
- no sheet insert, delete, rename, or move
- no copy, move, clipboard, load-time, import, or undo-like flows

## Named-Range Classes In Scope

The candidate named-range-sensitive slice is intentionally narrower than
"named ranges in general."

At entry, the proof cycle may evaluate only:

- global named ranges with a single-area target
- sheet-local named ranges with a single-area target
- names whose target area remains on one existing sheet after the edit
- names whose target expression can still be represented by one normalized
  target expression after the edit

The proof cycle may observe, but must not promote without separate evidence:

- sheet-local names
- formulas on sheets other than the edited sheet that depend on an admitted
  named range

The following stay explicitly out of scope:

- multi-area names
- names whose post-edit target requires union construction or scope transfer
- names that become invalid or ambiguous only because multiple scopes share
  the same lookup text
- named-range create, delete, rename, or scope-transfer mutations

## Formula Classes In Scope

Only ordinary scalar formulas are in scope.

At entry, the contract allows formulas that:

- reference a named single-cell target
- reference a named area target
- live on the edited sheet
- live on another sheet but depend on an admitted named range

The following stay out of scope:

- shared-group formulas
- matrix formulas
- formulas whose success depends on hidden Calc repair
- formulas whose structural behavior widens into external-reference-sensitive
  or token-container-sensitive handling

## Candidate Promotion Surface

The only slice that may be considered for live rollout promotion by the end of
this plan is:

- global named ranges
- single-area targets only
- ordinary scalar formulas only
- same-sheet structural edits only
- same exact-verification and rollback standard as the current admitted narrow
  structural rollout

Everything else in this proof cycle is evidence, not implied promotion.

## Validation-Only Surface

The proof cycle is allowed to run the following classes through the structural
authority harness in validation-only mode:

- sheet-local named ranges with a single-area target
- off-sheet formulas that depend on an admitted named range
- named area targets whose observable behavior still stays inside the ordinary
  scalar formula slice

Validation-only classification is acceptable closeout evidence. It is not a
partial promotion.

## Immediate Defer Triggers

The affected scenario must be deferred immediately if any of the following
appear:

- shared-group repair
- non-local structural repair outside the current exact-verification model
- hidden Calc repair on the supposed happy path
- scope ambiguity that cannot be represented by one engine-owned named-range
  descriptor
- multi-area target rewriting
- queue, computational, or graph mismatch after a supposedly admitted case
- rollback that is not deterministic

## Allowed Closeout Outcomes

This contract allows three honest answers:

- a bounded global single-area named-range-sensitive slice is promotable
- named-range-sensitive structural behavior is validation-only
- named-range-sensitive structural behavior remains deferred

It does not allow a closeout result that quietly widens beyond this contract.
