# Computational Substrate Shared-Group Non-Structural Named-Range-Combined Contract

Status: frozen contract for the bounded named-range-combined closeout

## Candidate Under Test

This closeout tests one additional bounded non-structural shared-group
family:

- same-sheet shareable shared-group `SetFormula`
- touched address already shared before the mutation
- touched address still shared after the mutation
- `SameTextPreserve` only
- formulas use only global single-area named ranges
- named-range descriptors stay stable:
  - no add
  - no remove
  - no rename
  - no retarget
- all relevant named-range consumers stay on the same sheet
- clean baseline only

Representative shape:

- a shared formula such as `=COUNTA(Metrics)+A2` is written back with
  identical source text
- the shared-group topology remains unchanged
- the only additional proof obligation is exact named-range-linked closure

## Carried-Forward Admitted Surface

The following shared-group families remain admitted from earlier cycles:

- structural `Preserve`, `Split`, and `Rebuild`
- non-structural member-exit `SetScalarValue`
- non-structural member-exit `SetFormula`
- non-structural member-exit `ClearCell`
- non-structural same-text preserve `SetFormula`
- non-structural edge-regroup `SetFormula`
- non-structural gap-closing merge `SetFormula`
- non-structural edge replacement-merge `SetFormula`
- non-structural one-sided adjacent insertion `SetFormula`

## Explicitly Deferred

This cycle does not admit:

- named-range-combined member-exit, even on the same bounded global
  single-area surface
- sheet-local named ranges
- multi-area named ranges
- scope-ambiguous named ranges
- named-range add, remove, rename, or retarget
- off-sheet named-range consumers
- named-range-combined regroup
- named-range-combined merge
- named-range-combined multi-group collapse
- repair-sensitive normalization
- off-sheet shared-group behavior

## Required Live Standard

The bounded preserve family would admit only if all of the following stay
exact:

- shared-group topology
- named-range descriptor inventory
- dependency snapshot and recalc queue inputs
- computational after-shadow
- named-range comparison surface
- graph and IR after-state
- lifecycle and mutation-entry live verification

Anything outside that exact bounded family remains out of contract.

## Closeout Result

This cycle kept the whole named-range-combined family deferred.

The candidate under test did not reach live admission.
