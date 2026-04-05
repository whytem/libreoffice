# Computational Substrate Mutation Entry Contract

Status: frozen admitted mutation-entry contract

## Purpose

This note freezes the exact admitted mutation-entry surface for
[COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md)
before implementation proceeds.

The goal of this contract is to avoid a fuzzy boundary where Calc still
quietly remains the real mutation-entry authority on the admitted slice.

## Admitted Workbook Slice

The admitted workbook slice for this proof cycle remains the same bounded
surface already proven by the earlier resident-state and formula-lifetime
work:

- clean baseline only
- ordinary scalar formulas only
- no shared groups
- no named-range-sensitive structural behavior
- no sheet-wide structural edits
- no off-sheet structural consumers
- no token-container migration

If the workbook or mutation falls outside that slice, this cycle must reject
or defer rather than silently widening the boundary.

## Admitted Mutation-Entry Operations

The admitted mutation-entry operations for this cycle are:

- `SetScalarValue`
- `SetFormula`
- `ClearCell`
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`

The intent of this cycle is not to widen the mutation classes beyond the
already-admitted lifecycle and structural surface. It is to move the entry
authority for that existing admitted surface into the engine.

## Engine-Owned Responsibilities In This Cycle

During this cycle, the engine may own:

- normalized admitted mutation request records
- request-to-mutation classification and routing
- admitted before/after-state decisions
- resident cell, formula-lifetime, and wiring after-state
- graph and queue after-state
- exact after-state verification targets

That means the engine should decide what the admitted mutation means and what
the admitted after-state must be.

## Retained Calc-Owned Responsibilities In This Cycle

Calc intentionally remains host-owned for:

- document mutation APIs
- temporary application of raw mutation requests to the live document
- live object realization from engine-owned resident state
- final rollback
- all out-of-contract workbook and mutation classes

This is still a bounded migration step. It does not claim broad host
independence for mutation entry.

## Exact Success Standard

This cycle counts as exact success only if:

- the admitted mutation request enters through the engine-owned request
  surface first
- the engine classifies and routes the request without relying on hidden
  Calc-only mutation intent
- the engine-owned resident cell, formula-lifetime, and wiring state remain
  exact after the admitted mutation
- Calc can apply and realize the resulting engine-owned state and still pass:
  - exact queue verification
  - exact computational verification
  - exact graph verification
- rollback remains explicit when verification fails

## Immediate Defer Or Reject Conditions

This cycle must reject, remain validation-only, or defer if any of the
following are required for success:

- hidden widening into shared-group-sensitive behavior
- hidden widening into named-range-sensitive behavior
- hidden widening into sheet-wide structural behavior
- implicit reuse of Calc-local token-container ownership as the real entry
  authority
- optimistic acceptance of queue, computational, or graph mismatches
- rollback being silently weakened or skipped

## Working Boundary

If this cycle succeeds, the working admitted boundary becomes:

- engine-owned mutation entry intent on the admitted slice
- engine-owned resident after-state and verification targets on that slice
- Calc-owned apply, live realization, and rollback

That is the only boundary this contract is willing to prove.
