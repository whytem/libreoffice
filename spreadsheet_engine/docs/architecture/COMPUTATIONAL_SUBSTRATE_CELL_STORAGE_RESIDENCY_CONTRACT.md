# Computational Substrate Cell Storage Residency Contract

Status: frozen authority contract

## Purpose

This note freezes the exact authority surface for the first cell-storage
residency pilot.

It exists to prevent the implementation from silently widening beyond the
currently admitted computational-substrate slice while the project tests
engine-resident cell storage for the first time.

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

## Engine-Owned Cell Storage In This Pilot

The engine-resident cell store must become authoritative for the following
value-semantic state on the admitted slice:

- admitted scalar-cell population
- admitted formula-cell population
- admitted formula payload text and cached value state
- stable admitted cell address identity and ordering
- admitted-sheet storage generation after each applied mutation

The admitted cell store may reuse engine-owned in-memory workbook state as
its backing representation, but it must not keep hidden Calc pointer
ownership or deferred host object identity.

## Retained Engine-Owned Decision Surfaces

This pilot does not replace the already-proven storage-and-wiring authority
boundary. The engine must still remain authoritative on the admitted slice
for:

- mutable computational sidecar state
- graph and wiring target sets
- formula-tree and formula-track target sets
- recalc-plan output for the same mutation

Cell residency becomes an additional engine-owned surface, not a replacement
for the earlier admitted boundary.

## Calc-Owned Host Surfaces In This Pilot

Calc remains the temporary host for:

- `ScDocument` mutation entry APIs
- formula-cell object lifetime
- live mirrored document storage outside the admitted slice
- live broadcaster/listener container residency
- formula-tree and formula-track container residency
- final rollback after failed verification

Calc may mirror the admitted engine-owned cell state into the live document
model, but it does not regain authority for deciding the admitted after-state.

## Verification Standard

The engine-driven path must satisfy all of the following on the admitted
slice:

- exact admitted cell-state comparison after mirroring
- exact computational comparison
- exact graph comparison
- exact queue comparison
- explicit rollback on verification failure
- explicit rollback or reject on repair-detected divergence

Execution-IR comparison remains observational in this pilot. It informs the
closeout decision but does not become a new rollout gate here.

## Out Of Contract

The following remain out of contract:

- shared-group lifecycle or storage residency
- named-range-sensitive structural behavior
- sheet-local or scope-ambiguous named ranges
- sheet insert/delete/rename/move
- external-reference-sensitive storage residency
- host-only service integrations
- live broadcaster/listener container residency migration
- broad `ScDocument` storage migration
- `ScTokenArray` ownership migration

## Success Condition

This contract is satisfied only if the engine can own the admitted-slice cell
store as resident mutable state, Calc can mirror that state into the retained
live document model, and the combined storage-plus-wiring path still passes
exact bounded verification without rebuilding admitted cell residency from
Calc after each mutation.
