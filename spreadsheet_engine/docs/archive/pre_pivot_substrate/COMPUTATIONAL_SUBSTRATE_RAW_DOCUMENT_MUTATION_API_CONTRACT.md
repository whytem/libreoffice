# Computational Substrate Raw Document Mutation API Contract

Status: frozen raw-document-mutation contract

## Purpose

This note freezes the exact proof surface for
[COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md)
before raw document mutation implementation work begins.

The purpose of this contract is to keep the next cycle focused on the last
remaining primitive document-mutation boundary on the admitted slice:

- engine-authored primitive mutation records for admitted mutations
- exact primitive mutation execution on the admitted slice
- not broader workbook mutation migration
- not a broad `ScDocument` rewrite

This is a raw document mutation reassessment, not a blanket replacement of
all Calc mutation entry and execution APIs.

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

## Primitive Mutation Classes Under Reassessment

This cycle is limited to the admitted primitive mutation classes that still
sit between engine-authored live apply plans and the retained Calc host
execution shell:

- admitted scalar overwrite and scalar text or number assignment
- admitted formula replace and clear
- admitted single-sheet row structural mutations
- admitted single-sheet column structural mutations
- admitted runtime combinations of primitive mutation execution plus
  realization, verification, and rollback record consumption

The cycle may introduce an explicit engine-authored primitive mutation
record and engine-authored primitive apply verdicts for this bounded
surface, but it must not claim broad host independence outside that record.

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
- admitted live object-realization records
- admitted rollback records
- admitted raw mutation records
- admitted live apply plans

Primitive raw document mutation execution becomes an additional bounded
proof surface layered on top of those settled resident, decision,
realization, rollback, raw-mutation, and live-apply surfaces.

## Retained Calc-Owned Host Surfaces

Calc intentionally remains host-owned in this cycle for:

- all out-of-contract workbook and mutation classes
- the broader live document shell outside the admitted slice
- primitive realization host operations around admitted primitive mutation
  execution
- primitive rollback host operations around admitted primitive mutation
  execution
- the final verification host shell around those primitive operations

Calc may still execute primitive mutation instructions in this cycle, but
it must not quietly reconstruct mutation intent or sequencing with hidden
local authority.

## Exact Success Standard

Raw document mutation migration counts as success only if all of the
following hold on the admitted slice:

- admitted primitive mutation instructions are explicit and engine-authored
- admitted primitive execution does not mutate beyond the engine-authored
  record
- queue comparison remains exact after live apply
- computational comparison remains a full match after live apply
- graph comparison remains exact and full-match after live apply
- admitted realization and rollback records remain exact and green when
  triggered from the primitive mutation path
- dirty-baseline rejection, repair-detected behavior, and rollback remain
  explicit and green

This cycle does not succeed merely because the live document looks usable
after applying a primitive mutation record. The admitted proof lanes must
remain exact.

## Immediate Hybrid Or Defer Triggers

This cycle must immediately keep primitive mutation execution hybrid or
defer broader raw document mutation migration if the proof surface requires
any of the following:

- widening beyond the admitted scalar and narrow structural slice
- hidden Calc-only primitive mutation intent not representable by an
  engine-authored record
- reintroducing host-owned after-state, realization, rollback, or live
  apply decisions
- shared-group-sensitive, named-range-sensitive, off-sheet, or sheet-wide
  classes
- a fix that weakens exact queue, computational, graph, replay,
  realization, or rollback verification

## Required Closeout Shape

This contract is satisfied only if the closeout ends in one of these
explicit states:

- engine-authored admitted-slice raw document mutation migration proceeds
- admitted raw document mutation remains hybrid with one clearly bounded
  reason
- broader raw document mutation migration is deferred again with explicit
  reasons

Anything less explicit than that is out of contract.
