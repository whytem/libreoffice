# Computational Substrate Primitive Execution Host Contract

Status: frozen primitive-execution contract

## Purpose

This note freezes the exact proof surface for
[COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md)
before primitive-execution implementation work begins.

The purpose of this contract is to keep the next cycle focused on the last
remaining admitted-slice primitive host shell around already-engine-authored
state:

- engine-authored primitive execution records or plans for admitted mutations
- exact admitted primitive execution of resident state after apply or reject
- not broader workbook mutation migration
- not a broad `ScDocument` rewrite

This is a primitive-execution reassessment, not a blanket replacement of all
Calc mutation, realization, rollback, and verification behavior.

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

## Primitive Execution Classes Under Reassessment

This cycle is limited to the admitted primitive execution classes that still
sit between engine-authored raw-document-mutation, realization, rollback, and
final-verification identity and the retained Calc host shell:

- admitted low-level scalar overwrite and clear execution
- admitted low-level formula replace execution
- admitted low-level single-sheet row structural execution
- admitted low-level single-sheet column structural execution
- admitted primitive realization and rollback call sequencing
- admitted runtime combinations of raw mutation, raw document mutation, live
  apply, primitive realization or rollback, final verification, and primitive
  execution

The cycle may introduce an explicit engine-authored primitive execution
record, plan, or verdict surface for this bounded proof area, but it must not
claim broad host independence outside that surface.

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
- admitted final verification records and observations

Primitive execution becomes an additional bounded proof surface layered on top
of those settled resident, mutation, realization, rollback, live-apply,
primitive-realization, primitive-rollback, and final-verification surfaces.

## Retained Calc-Owned Host Surfaces

Calc intentionally remains host-owned in this cycle for:

- all out-of-contract workbook and mutation classes
- the broader live document shell outside the admitted slice
- the primitive host calls that still execute admitted low-level document
  mutation, realization, and rollback work after the engine-authored records
  have been built

Calc may still execute primitive operations in this cycle, but it must not
quietly reconstruct primitive sequencing or acceptance criteria with hidden
local authority.

## Exact Success Standard

Primitive execution migration counts as success only if all of the following
hold on the admitted slice:

- admitted primitive execution identity is explicit and engine-authored
- admitted primitive execution does not execute beyond the engine-authored
  record or plan
- queue comparison remains exact after apply and after rollback
- computational comparison remains a full match after apply and after
  rollback
- graph comparison remains exact and full-match after apply and after
  rollback
- admitted raw mutation, raw document mutation, realization, rollback, live
  apply, primitive realization or rollback, and final verification records
  remain exact and green when primitive execution is triggered from that path
- dirty-baseline rejection, repair-detected behavior, and rollback remain
  explicit and green

This cycle does not succeed merely because the live document looks usable
after applying or restoring admitted state. The admitted primitive execution
lanes must remain exact.

## Immediate Hybrid Or Defer Triggers

This cycle must immediately keep primitive execution hybrid or defer broader
migration if the proof surface requires any of the following:

- widening beyond the admitted scalar and narrow structural slice
- hidden Calc-only primitive execution intent not representable by an
  engine-authored record or plan
- reintroducing host-owned mutation, realization, rollback, or final
  verification decisions
- shared-group-sensitive, named-range-sensitive, off-sheet, or sheet-wide
  classes
- a fix that weakens exact queue, computational, graph, replay,
  realization, rollback, or verification proof

## Required Closeout Shape

This contract is satisfied only if the closeout ends in one of these explicit
states:

- engine-authored admitted-slice primitive execution migration proceeds
- admitted primitive execution remains hybrid with one clearly bounded reason
- broader primitive execution migration is deferred again with explicit
  reasons

Anything less explicit than that is out of contract.
