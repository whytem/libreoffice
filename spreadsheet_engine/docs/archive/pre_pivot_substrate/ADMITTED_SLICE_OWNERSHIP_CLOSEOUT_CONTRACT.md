# Admitted Slice Ownership Closeout Contract

Status: frozen ownership-closeout contract

## Purpose

This note freezes the exact proof surface for
[ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md)
before ownership-closeout implementation work begins.

The purpose of this contract is to keep the next cycle focused on the last
remaining retained host shell on the already-admitted slice:

- the primitive host-call executor around admitted low-level mutation,
  realization, and rollback work
- not broader workbook-class widening
- not a broad `ScDocument` rewrite

This is an ownership-closeout reassessment for the current admitted slice,
not a plan for widening into harder workbook classes.

## In-Scope Workbook And Mutation Surface

The admitted slice remains fixed to the already-proven bounded surface:

- ordinary scalar formulas only
- clean baseline only
- `SetScalarValue`
- `SetFormula`
- `ClearCell`
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`
- no shared groups
- no named-range-sensitive structural behavior
- no sheet insert, delete, rename, or move
- no copy, move, clipboard, load-time, or undo-like flows

If the workbook or mutation leaves that slice, this cycle must reject,
rollback, or defer instead of silently widening.

## Primitive Host-Call Executor Classes Under Reassessment

This closeout is limited to the admitted primitive host-call execution
classes that still sit underneath already-engine-authored state:

- low-level scalar overwrite and clear execution
- low-level formula replace execution
- low-level single-sheet row structural execution
- low-level single-sheet column structural execution
- low-level primitive realization execution
- low-level primitive rollback execution
- admitted runtime combinations of raw mutation, raw document mutation,
  primitive execution, realization, rollback, live apply, and final
  verification on that bounded slice

The cycle may introduce an explicit engine-authored primitive host-call
executor plan, observation, or verdict surface for this bounded proof area,
but it must not claim broader host independence outside that area.

## Engine-Owned Surfaces That Stay Settled

This closeout assumes the engine already remains authoritative on the
admitted slice for:

- resident cell storage
- resident wiring containers
- admitted formula-cell lifetime decisions
- admitted mutation-entry request shape, routing, and after-state decisions
- admitted raw mutation records
- admitted raw document mutation records and primitive apply verdicts
- admitted live object-realization records
- admitted rollback records
- admitted live apply plans
- admitted primitive realization and primitive rollback records and apply
  verdicts
- admitted final verification records and observations
- admitted primitive execution plans and observations

Ownership-closeout only reassesses the retained primitive host-call executor
around those settled engine-authored surfaces.

## Retained Calc-Owned Surfaces In This Cycle

Calc intentionally remains host-owned in this cycle for:

- all workbook and mutation classes outside the admitted slice
- the broader live document shell outside the admitted slice
- the thin primitive host-call executor that still performs low-level
  admitted mutation, realization, and rollback calls

Calc may still issue primitive calls in this cycle, but it must not quietly
reconstruct host-call identity, sequencing, or acceptance criteria with
hidden local authority.

## Exact Success Standard

Ownership-closeout counts as success only if all of the following hold on
the admitted slice:

- admitted primitive host-call execution identity is explicit and
  engine-authored
- admitted apply and rollback lanes both consume that same executor surface
- Calc does not introduce hidden local sequencing beyond that executor
  surface
- queue comparison remains exact after apply and after rollback
- computational comparison remains a full match after apply and after
  rollback
- graph comparison remains exact and full-match after apply and after
  rollback
- admitted resident storage, resident wiring, lifetime, mutation-entry, raw
  mutation, raw document mutation, primitive execution, realization,
  rollback, live apply, and final verification surfaces remain exact and
  green when the executor is exercised
- dirty-baseline rejection, repair-detected behavior, and rollback remain
  explicit and green

This cycle does not succeed merely because the live document remains usable.
The closeout must show that the admitted slice no longer relies on hidden
Calc executor authority.

## Immediate Hybrid Or Defer Triggers

This cycle must immediately remain hybrid or defer if the proof surface
requires any of the following:

- widening beyond the admitted scalar and narrow structural slice
- hidden Calc-only primitive host-call intent not representable by an
  engine-authored executor plan
- reintroducing host-owned mutation, realization, rollback, or verification
  identity that is not already settled
- shared-group-sensitive, named-range-sensitive, off-sheet, or sheet-wide
  classes
- weakening exact queue, computational, graph, replay, realization,
  rollback, or verification proof

## Required Closeout Shape

This contract is satisfied only if the closeout ends in one of these
explicit states:

- admitted-slice substantive ownership is complete
- admitted-slice ownership remains hybrid with one clearly bounded reason
- admitted-slice ownership closeout is deferred again with explicit reasons

Anything less explicit than that is out of contract.
