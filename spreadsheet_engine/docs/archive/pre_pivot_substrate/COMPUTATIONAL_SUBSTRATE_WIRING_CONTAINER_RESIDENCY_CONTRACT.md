# Computational Substrate Wiring Container Residency Contract

Status: frozen authority contract

## Purpose

This note freezes the exact authority surface for the first live
wiring-container residency pilot.

It exists to prevent the implementation from silently widening beyond the
currently admitted computational-substrate slice while the project tests
engine-resident live wiring containers for the first time.

## In-Scope Workbook And Mutation Surface

The pilot is limited to the already-admitted narrow rollout boundary:

- ordinary scalar formulas only
- clean baseline only
- admitted scalar lifecycle mutations
- admitted single-sheet structural mutations
- no shared groups
- no named-range-sensitive structural behavior
- no sheet-level structural edits
- no move, copy, load-time, clipboard, or undo-like flows

The admitted mutation classes are:

- `SetScalarValue`
- `SetFormula`
- `ClearCell`
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`

## Engine-Owned Wiring Containers In This Pilot

The engine-resident admitted wiring store must become authoritative for the
following value-semantic state on the admitted slice:

- admitted listener-edge population
- admitted broadcaster-node population
- admitted formula-tree realized order
- admitted formula-track realized order
- admitted wiring-container generation after each applied mutation

The store must remain free of hidden Calc pointer ownership, deferred host
object identity, or reliance on live `ScDocument` container residency as the
source of truth.

## Retained Engine-Owned Decision Surfaces

This pilot extends, but does not replace, the already-proven storage and
cell-residency boundary. The engine must still remain authoritative on the
admitted slice for:

- mutable computational sidecar state
- admitted resident cell storage
- graph and wiring target decisions
- recalc-plan output for the same mutation

Live wiring-container residency becomes an additional engine-owned surface,
not a replacement for the earlier admitted boundary.

## Calc-Owned Host Surfaces In This Pilot

Calc remains the temporary host for:

- `ScDocument` mutation entry APIs
- formula-cell object lifetime
- live mirrored document storage outside the admitted slice
- realization of engine-owned admitted wiring state into host containers
- final rollback after failed verification

Calc may realize the admitted engine-owned wiring state into the retained live
document model, but it does not regain authority for deciding the admitted
listener, broadcaster, formula-tree, or formula-track after-state.

## Verification Standard

The engine-driven path must satisfy all of the following on the admitted
slice:

- exact admitted wiring-container comparison after realization
- exact admitted cell-storage comparison
- exact computational comparison
- exact graph comparison
- exact queue comparison
- explicit rollback on verification failure
- explicit rollback or reject on repair-detected divergence

Execution-IR comparison remains observational in this pilot. It informs the
closeout decision but does not become a new rollout gate here.

## Out Of Contract

The following remain out of contract:

- shared-group lifecycle or wiring-container residency
- named-range-sensitive structural behavior
- sheet-local or scope-ambiguous named ranges
- sheet insert/delete/rename/move
- external-reference-sensitive wiring-container residency
- host-only service integrations
- formula-cell object lifetime migration
- broad `ScDocument` dependency-container migration
- `ScTokenArray` ownership migration

## Success Condition

This contract is satisfied only if the engine can own the admitted-slice live
wiring containers as resident mutable state, Calc can realize that state into
the retained host document model, and the combined cell-plus-wiring path still
passes exact bounded verification without rebuilding admitted wiring
containers from Calc after each mutation.
