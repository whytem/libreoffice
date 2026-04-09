# Computational Substrate Primitive Realization And Rollback Contract

Status: frozen primitive-realization-rollback contract

## Purpose

This note freezes the exact proof surface for
[COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md)
before primitive realization and rollback implementation work begins.

The purpose of this contract is to keep the next cycle focused on the last
remaining primitive host shell on the admitted slice:

- engine-authored primitive realization records for admitted mutations
- engine-authored primitive rollback records for admitted mutations
- exact primitive realization and rollback execution on the admitted slice
- not broader workbook mutation migration
- not a broad `ScDocument` rewrite

This is a primitive realization-and-rollback reassessment, not a blanket
replacement of all Calc realization, rollback, and verification behavior.

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

## Primitive Realization And Rollback Classes Under Reassessment

This cycle is limited to the admitted primitive realization and rollback
classes that still sit between engine-authored raw document mutation and the
retained Calc host execution shell:

- admitted formula-cell lifetime realization
- admitted resident cell-storage mirroring during realization
- admitted resident wiring realization during realization
- admitted rollback restore of realized objects and resident state
- admitted runtime combinations of primitive realization and rollback plus
  exact verification record consumption

The cycle may introduce explicit engine-authored primitive realization and
primitive rollback records for this bounded surface, but it must not claim
broad host independence outside those records.

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

Primitive realization and rollback execution become an additional bounded
proof surface layered on top of those settled resident, decision,
realization-record, rollback-record, and live-apply surfaces.

## Retained Calc-Owned Host Surfaces

Calc intentionally remains host-owned in this cycle for:

- all out-of-contract workbook and mutation classes
- the broader live document shell outside the admitted slice
- the final verification host shell around admitted primitive realization
  and rollback execution

Calc may still execute primitive realization and rollback instructions in
this cycle, but it must not quietly reconstruct realization or rollback
intent with hidden local authority.

## Exact Success Standard

Primitive realization and rollback migration counts as success only if all
of the following hold on the admitted slice:

- admitted primitive realization and rollback instructions are explicit and
  engine-authored
- admitted primitive execution does not mutate beyond the engine-authored
  records
- queue comparison remains exact after apply and after rollback
- computational comparison remains a full match after apply and after
  rollback
- graph comparison remains exact and full-match after apply and after
  rollback
- admitted live apply and raw document mutation records remain exact and
  green when primitive realization and rollback are triggered from that path
- dirty-baseline rejection, repair-detected behavior, and rollback remain
  explicit and green

This cycle does not succeed merely because the live document looks usable
after applying or restoring state. The admitted proof lanes must remain
exact.

## Immediate Hybrid Or Defer Triggers

This cycle must immediately keep primitive realization and rollback hybrid
or defer broader migration if the proof surface requires any of the
following:

- widening beyond the admitted scalar and narrow structural slice
- hidden Calc-only realization or rollback intent not representable by an
  engine-authored primitive record
- reintroducing host-owned mutation, raw-document-mutation, or live-apply
  decisions
- shared-group-sensitive, named-range-sensitive, off-sheet, or sheet-wide
  classes
- a fix that weakens exact queue, computational, graph, replay,
  realization, or rollback verification

## Required Closeout Shape

This contract is satisfied only if the closeout ends in one of these
explicit states:

- engine-authored admitted-slice primitive realization and rollback
  migration proceeds
- admitted primitive realization and rollback remain hybrid with one
  clearly bounded reason
- broader primitive realization and rollback migration is deferred again
  with explicit reasons

Anything less explicit than that is out of contract.
