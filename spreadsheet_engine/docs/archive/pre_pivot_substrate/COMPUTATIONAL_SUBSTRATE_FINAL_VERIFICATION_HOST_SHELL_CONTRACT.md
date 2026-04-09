# Computational Substrate Final Verification Host-Shell Contract

Status: frozen final-verification contract

## Purpose

This note freezes the exact proof surface for
[COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md)
before final verification implementation work begins.

The purpose of this contract is to keep the next cycle focused on the last
remaining admitted-slice host shell around already-engine-authored state:

- engine-authored final verification records or plans for admitted
  mutations
- exact admitted verification of resident state after apply or reject
- not broader workbook mutation migration
- not a broad `ScDocument` rewrite

This is a final-verification reassessment, not a blanket replacement of all
Calc verification and execution behavior.

## In-Scope Workbook And Mutation Surface

The admitted workbook slice remains the already-proven narrow authority
boundary:

- ordinary scalar formulas only
- clean baseline only
- admitted scalar lifecycle mutations
- admitted scalar mutation entry
- admitted single-sheet structural mutations
- no shared groups
- no named-range-sensitive structural behavior
- no sheet insert, delete, rename, or move
- no move, copy, load-time, clipboard, or undo-like flows

The admitted mutation classes remain:

- `SetScalarValue`
- `SetFormula`
- `ClearCell`
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`

If the workbook or mutation leaves that slice, this cycle must reject,
rollback, or defer instead of silently widening.

## Final Verification Classes Under Reassessment

This cycle is limited to the admitted final verification classes that still
sit between engine-authored primitive realization/rollback and the retained
Calc host shell:

- admitted verification of resident cell state after apply
- admitted verification of resident wiring state after apply
- admitted verification of formula-cell lifetime realization after apply
- admitted verification of primitive rollback restore after reject
- admitted runtime combinations of raw mutation, raw document mutation,
  live apply, primitive realization/rollback, and final verification

The cycle may introduce an explicit engine-authored final verification
record, plan, or verdict surface for this bounded proof area, but it must
not claim broad host independence outside that surface.

## Engine-Owned Surfaces That Stay Settled

This cycle assumes the engine already remains authoritative on the admitted
slice for:

- resident cell storage
- resident wiring containers
- admitted formula-cell lifetime decisions
- mutable computational state
- graph, wiring, and queue decisions
- admitted scalar mutation-entry request shape, routing, and after-state
  decisions
- admitted raw mutation records
- admitted raw document mutation records and primitive apply verdicts
- admitted live object-realization records
- admitted rollback records
- admitted live apply plans
- admitted primitive realization and primitive rollback records and apply
  verdicts

Final verification becomes an additional bounded proof surface layered on
top of those settled resident, decision, mutation, realization, rollback,
and primitive-execution surfaces.

## Retained Calc-Owned Host Surfaces

Calc intentionally remains host-owned in this cycle for:

- all out-of-contract workbook and mutation classes
- the broader live document shell outside the admitted slice
- primitive execution host operations that the final verification record
  still observes

Calc may still execute primitive operations in this cycle, but it must not
quietly reconstruct verification intent or acceptance criteria with hidden
local authority.

## Exact Success Standard

Final verification migration counts as success only if all of the following
hold on the admitted slice:

- admitted final verification identity is explicit and engine-authored
- admitted verification does not accept or reject beyond the
  engine-authored record or plan
- queue comparison remains exact after apply and after rollback
- computational comparison remains a full match after apply and after
  rollback
- graph comparison remains exact and full-match after apply and after
  rollback
- admitted raw mutation, raw document mutation, live apply, primitive
  realization, and primitive rollback records remain exact and green when
  final verification is triggered from that path
- dirty-baseline rejection, repair-detected behavior, and rollback remain
  explicit and green

This cycle does not succeed merely because the live document looks usable
after applying or restoring admitted state. The admitted verification lanes
must remain exact.

## Immediate Hybrid Or Defer Triggers

This cycle must immediately keep final verification hybrid or defer broader
migration if the proof surface requires any of the following:

- widening beyond the admitted scalar and narrow structural slice
- hidden Calc-only verification intent not representable by an
  engine-authored record or plan
- reintroducing host-owned mutation, realization, rollback, or primitive
  execution decisions
- shared-group-sensitive, named-range-sensitive, off-sheet, or sheet-wide
  classes
- a fix that weakens exact queue, computational, graph, replay,
  realization, or rollback verification

## Required Closeout Shape

This contract is satisfied only if the closeout ends in one of these
explicit states:

- engine-authored admitted-slice final verification migration proceeds
- admitted final verification remains hybrid with one clearly bounded
  reason
- broader final verification migration is deferred again with explicit
  reasons

Anything less explicit than that is out of contract.
