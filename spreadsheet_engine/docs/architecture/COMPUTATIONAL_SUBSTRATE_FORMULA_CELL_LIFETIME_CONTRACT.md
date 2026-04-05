# Computational Substrate Formula-Cell Lifetime Contract

Status: frozen authority contract

## Purpose

This note freezes the exact authority surface for the first admitted-slice
formula-cell lifetime pilot.

It exists to prevent the implementation from silently widening beyond the
currently admitted computational-substrate slice while the project tests
engine-owned formula-cell object lifetime for the first time.

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

## Engine-Owned Formula-Cell Lifetime In This Pilot

The engine-owned admitted lifetime store must become authoritative for the
following value-semantic state on the admitted slice:

- admitted formula-cell population
- admitted formula-cell create decisions
- admitted formula-cell replace decisions
- admitted formula-cell remove decisions
- stable admitted formula-cell address identity and ordering
- admitted formula-cell lifetime generation after each applied mutation

The store must remain free of hidden Calc pointer ownership, deferred host
object identity, or reliance on live `ScFormulaCell` objects as the source of
truth.

## Retained Engine-Owned Resident State

This pilot extends, but does not replace, the already-proven resident state
boundary. The engine must still remain authoritative on the admitted slice
for:

- resident admitted cell storage
- resident admitted wiring containers
- mutable computational state
- graph, wiring, and recalc decisions

Formula-cell lifetime becomes an additional engine-owned surface, not a
replacement for the earlier admitted boundary.

## Calc-Owned Host Surfaces In This Pilot

Calc remains the temporary host for:

- `ScDocument` mutation entry APIs
- realization of engine-owned admitted formula-cell lifetime into live
  `ScFormulaCell` objects
- final rollback after failed verification

Calc may realize the admitted engine-owned lifetime state into the retained
live document model, but it does not regain authority for deciding the
admitted formula-cell after-state.

## Verification Standard

The engine-driven path must satisfy all of the following on the admitted
slice:

- exact admitted formula-cell lifetime comparison after realization
- exact admitted cell-storage comparison
- exact admitted wiring-container comparison
- exact computational comparison
- exact graph comparison
- exact queue comparison
- explicit rollback on verification failure
- explicit rollback or reject on repair-detected divergence

Execution-IR comparison remains observational in this pilot. It informs the
closeout decision but does not become a new rollout gate here.

## Out Of Contract

The following remain out of contract:

- shared-group lifecycle or formula-cell lifetime
- named-range-sensitive structural behavior
- sheet-local or scope-ambiguous named ranges
- sheet insert, delete, rename, or move
- external-reference-sensitive formula-cell lifetime
- host-only service integrations
- direct engine-owned mutation entry
- broad `ScDocument` object lifetime migration
- `ScTokenArray` ownership migration

## Success Condition

This contract is satisfied only if the engine can own the admitted-slice
formula-cell lifetime as resident mutable state, Calc can realize that state
into live `ScFormulaCell` objects, and the combined lifetime-plus-cell-plus-
wiring path still passes exact bounded verification without rebuilding
admitted formula-cell lifetime from Calc after each mutation.
