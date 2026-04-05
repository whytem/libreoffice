# Computational Substrate Live Apply-Shell Contract

Status: frozen live-apply-shell contract

## Purpose

This note freezes the exact proof surface for
[COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md)
before live-apply-shell implementation work begins.

The purpose of this contract is to keep the next cycle focused on the last
remaining host-owned sequencing surface on the admitted slice:

- engine-authored raw mutation record consumption
- engine-authored realization record consumption
- exact verification sequencing
- engine-authored rollback record consumption
- not broader workbook mutation migration
- not a broad document-host rewrite

This is an apply-shell reassessment, not a blanket replacement of all
`ScDocument` mutation APIs.

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

## Apply-Shell Classes Under Reassessment

This cycle is limited to the admitted apply-shell classes that currently sit
between engine-authored records and the retained Calc host shell:

- admitted raw mutation record consumption
- admitted object-realization record consumption
- admitted exact verification sequencing
- admitted rollback record consumption
- admitted runtime combinations of those stages on scalar and narrow
  structural lanes

The cycle may introduce an explicit engine-authored apply-plan record for
this bounded surface, but it must not claim broad host independence outside
that record.

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

Live apply sequencing becomes an additional bounded proof surface layered on
top of those settled resident, decision, realization, rollback, and
raw-mutation surfaces.

## Retained Calc-Owned Host Surfaces

Calc intentionally remains host-owned in this cycle for:

- all out-of-contract workbook and mutation classes
- the broader live document shell outside the admitted slice
- the underlying raw document mutation APIs
- primitive execution of the admitted raw mutation, realization, and
  rollback records
- the final verification host shell around those primitive operations

Calc may still execute the admitted apply plan in this cycle, but it must
not quietly reclaim sequencing authority with hidden local orchestration.

## Exact Success Standard

Live apply-shell migration counts as success only if all of the following
hold on the admitted slice:

- admitted live apply sequencing is explicit and engine-authored
- admitted raw mutation identity remains explicit and unchanged by host
  execution
- queue comparison remains exact after live apply
- computational comparison remains a full match after live apply
- graph comparison remains exact and full-match after live apply
- admitted realization and rollback records remain exact and green when
  triggered from the apply shell
- dirty-baseline rejection, repair-detected behavior, and rollback remain
  explicit and green

This cycle does not succeed merely because the live document looks usable
after the admitted records are executed. The admitted proof lanes must
remain exact.

## Immediate Hybrid Or Defer Triggers

This cycle must immediately keep the live apply shell hybrid or defer broader
apply-shell migration if the proof surface requires any of the following:

- widening beyond the admitted scalar and narrow structural slice
- hidden Calc-only sequencing not representable by an engine-authored apply
  plan
- reintroducing host-owned mutation intent, realization intent, or rollback
  intent
- shared-group-sensitive, named-range-sensitive, off-sheet, or sheet-wide
  classes
- a fix that weakens exact queue, computational, graph, replay,
  realization, or rollback verification

## Required Closeout Shape

This contract is satisfied only if the closeout ends in one of these
explicit states:

- engine-authored admitted-slice live apply-shell migration proceeds
- admitted live apply shell remains hybrid with one clearly bounded reason
- broader live apply-shell migration is deferred again with explicit reasons

Anything less explicit than that is out of contract.
