# Computational Substrate Named-Range Structural Evidence

Status: complete evidence note for the named-range widening plan

## Purpose

This note records the actual proof outcomes from the named-range-sensitive
structural widening cycle.

It is intentionally narrower than a rollout decision. Its job is to say what
was observed, what stayed stable, and what still diverges.

## Evidence Summary

The current proof surface separates into three classes:

- one bounded global single-area validation-only class that now enters the
  structural pilot
- several clean defer classes that still reject deterministically
- one deliberate divergence class that still produces repair-detected rollback

## Exact-Match Evidence

Exact prediction evidence exists on the standalone structural builder surface
for the bounded global single-area case:

- single-sheet `InsertColumns`
- global name
- single-area target
- ordinary scalar formulas

The checked-in standalone case proves that:

- the named-range target rewrite is predicted exactly on the computational
  shadow surface
- the validation-only structural transition is applicable
- the proof does not require broadening into shared-group or multi-area logic

This is the strongest positive result from the cycle.

## Normalized-Equivalent Evidence

No useful named-range-specific normalized-equivalent class was needed to make
the current proof work.

That is a good outcome. It means the current global single-area prediction
surface is either exact enough to proceed into validation or cleanly rejected.

## Validation-Only Evidence

Calc differential evidence now shows that the bounded global single-area case
can enter the validation harness without rejecting out-of-contract:

- global name
- single-area target
- same-sheet structural edit
- ordinary scalar formulas

This is still validation-only evidence, not rollout admission evidence. The
important fact is that the case now reaches the verdict-bearing structural
bridge instead of being rejected up front.

## Rollback And Repair Evidence

Two deterministic negative-path results are now checked in:

- dirty-baseline entry rejects with `RejectedDirtyBaseline`
- deliberate post-edit divergence on the global named-range case returns
  `RepairDetected` and rolls back

That means the named-range widening cycle preserved the existing safety model:

- no dirty-baseline authority
- no silent acceptance of divergent structural rewrites

## Deferred Classes

The proof cycle still leaves the following classes deferred:

- sheet-local names
- multi-area names
- scope-ambiguous name sets
- shared-group-sensitive structural behavior

The checked-in differential results already show that sheet-local and
multi-area cases behave differently enough from the bounded global single-area
slice that they should not be silently grouped together.

## Global Versus Sheet-Local Outcome

Yes, the current evidence is strong enough to split the surface:

- global single-area names are the only class that currently enters the
  validation-only structural pilot successfully
- sheet-local names still reject out-of-contract on the same mutation class

That split is material. Any future admission decision must treat global and
sheet-local named-range behavior as separate questions.

## What This Evidence Supports

This evidence supports only the following conclusion:

- keep named-range-sensitive structural behavior out of the live rollout
- keep the bounded global single-area slice as a validation-only surface
- defer sheet-local and multi-area classes explicitly

It does not support live rollout admission yet.
