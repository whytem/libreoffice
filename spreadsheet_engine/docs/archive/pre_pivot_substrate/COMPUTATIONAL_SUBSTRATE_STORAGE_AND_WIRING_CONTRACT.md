# Computational Substrate Storage And Wiring Contract

Status: frozen authority contract

## Purpose

This note freezes the exact authority surface for the first storage-and-wiring
pilot. It exists to prevent the implementation from silently widening beyond
the currently admitted computational-substrate slice.

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

## Engine-Owned Mutable Sidecar State

The engine-maintained mutable sidecar must become authoritative for the
following value-semantic state on the admitted slice:

- computational cell population
- formula-cell descriptors
- formula-tree membership
- formula-track membership
- normalized cell broadcaster projection
- normalized area broadcaster projection

The mutable sidecar may use engine-owned in-memory workbook state as its
backing store, but it must not keep hidden Calc pointer ownership.

## Engine-Owned Live Decision Outputs

The engine must produce explicit deltas for the retained host side on the
admitted slice:

- listener edge add/remove actions
- broadcaster node add/remove actions
- formula-tree add/remove actions
- formula-track add/remove actions
- recalc plan output for the same mutation

The pilot does not claim physical broadcaster-container ownership. It only
claims decision authority for the admitted deltas.

## Calc-Owned Host Surfaces In This Pilot

Calc remains the temporary host for:

- `ScDocument` storage and mutation APIs
- formula-cell object lifetime
- live broadcaster/listener container residency
- `StartListeningCell` / `EndListeningCell`
- `StartListeningArea` / `EndListeningArea`
- formula-tree and formula-track container realization
- final rollback after failed verification

## Verification Standard

The engine-driven path must satisfy all of the following on the admitted
slice:

- exact computational comparison
- exact graph comparison
- exact queue comparison
- explicit rollback on verification failure
- explicit rollback or reject on repair-detected divergence

Execution-IR comparison remains observational in this pilot. It informs the
closeout decision but does not become a new rollout gate here.

## Out Of Contract

The following remain out of contract:

- shared-group lifecycle or wiring
- named-range-sensitive structural behavior
- sheet-local or scope-ambiguous named ranges
- sheet insert/delete/rename/move
- external-reference-sensitive wiring
- host-only service integrations
- physical cell-storage residency migration
- physical broadcaster/listener container migration
- `ScTokenArray` ownership migration

## Success Condition

This contract is satisfied only if the engine can drive the admitted mutable
sidecar and graph-delta path without rebuilding from Calc after each mutation,
while Calc can realize the same live outcome through explicit engine-issued
deltas and still pass exact bounded verification.
