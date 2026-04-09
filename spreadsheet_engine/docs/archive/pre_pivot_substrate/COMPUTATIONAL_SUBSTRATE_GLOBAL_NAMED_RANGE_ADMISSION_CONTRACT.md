# Computational Substrate Global Named-Range Admission Contract

Status: completed contract note for the global named-range admission plan

## Purpose

This note freezes the exact promotion candidate for the global named-range
admission proof cycle.

The candidate is intentionally narrower than the full named-range structural
surface. It exists to answer one question only:

- whether the already-proven global single-area validation-only slice can
  satisfy the same live opt-in admission standard as the current structural
  rollout

## Candidate Slice

The only live-admission candidate frozen by this contract is:

- global named ranges only
- single-area targets only
- unambiguous workbook-wide descriptor resolution only
- single-sheet structural edits only
- `InsertRows`
- `DeleteRows`
- `InsertColumns`
- `DeleteColumns`
- ordinary scalar formulas only
- clean baseline only
- exact queue, computational, and graph verification
- repair-detected rollback required on divergence

This candidate remains bounded even when formulas that consume the global
name live on a different sheet. The mutation surface is still same-sheet
structural editing; the proof question is whether global-name consumers stay
homogeneous enough to join one promotion surface.

## Entry Preconditions

The live-candidate path is in contract only when all of the following are
true:

- the structural rollout gate is enabled
- the dedicated global named-range admission gate is enabled
- the captured baseline is clean
- every formula in scope is an ordinary scalar formula
- there are no shared groups before or after the mutation
- every named range in scope resolves to a single-area target
- there is no workbook-global name ambiguity after case-folded comparison

## Explicitly Out Of Contract

The following remain out of contract for this promotion cycle:

- sheet-local named ranges
- mixed global-and-local shadowing sets
- scope-ambiguous name sets
- multi-area named ranges
- 3D or cross-sheet named-range targets
- shared-group-sensitive structural behavior
- sheet insert, delete, rename, or move
- copy, move, clipboard, import, load-time, or undo-like flows
- broad named-range create, delete, rename, or scope-transfer authority

These classes may still appear in validation or reject lanes, but they are
not promotion candidates for this plan.

## Required Result Shape

The candidate can be admitted only if it demonstrates the same proof shape as
the current structural rollout surface:

- exact standalone structural prediction on the admitted slice
- exact Calc differential application on the admitted slice
- deterministic dirty-baseline rejection
- deterministic out-of-contract rejection
- deterministic repair-detected rollback when divergence is introduced

Normalized-equivalent results are allowed only if they are explicitly frozen
in the equivalence rules and do not hide semantic divergence.

## Immediate Reject Or Defer Triggers

Any of the following force immediate reject or defer rather than promotion:

- dirty baseline at entry
- repair-detected structural divergence
- scope ambiguity
- sheet-local name resolution
- multi-area target parsing
- shared-group interaction
- mutation classes outside the four admitted row/column edits
- any dependence on broader Calc repair that the current exact-verification
  model cannot represent

## Promotion Standard

This slice is promotable only when the evidence shows:

- exact or explicitly accepted normalized-equivalent behavior across the full
  bounded candidate matrix
- no hidden dependence on retained Calc repair on the admitted slice
- no need to split the candidate further just to make the happy path succeed

If those conditions are not met, the slice stays validation-only or is
deferred again with an explicit reason.
