# Computational Substrate Final Rollback Contract

Status: frozen final-rollback contract

## Purpose

This note freezes the exact proof surface for
[COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md)
before rollback implementation work proceeds.

The purpose of this contract is to keep the next cycle focused on the last
remaining host-owned boundary on the admitted slice:

- engine-authored rollback records for admitted mutations
- exact live restoration of admitted resident and realized state
- not broader mutation-shell migration
- not wider workbook classes

This is a rollback reassessment, not a broad mutation-host rewrite.

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

## Rollback Classes Under Reassessment

This cycle is limited to the admitted rollback classes that currently sit
between engine-owned resident and realized state and the retained Calc host
shell:

- rollback of admitted scalar mutation entry
- rollback of admitted formula replace and remove realization
- rollback of admitted row and column structural mutations
- rollback of admitted listener and broadcaster live participation
- rollback of admitted formula-tree and formula-track live participation

The cycle may introduce an explicit engine-authored rollback record for this
bounded surface, but it must not claim broad rollback ownership outside that
record.

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

Rollback becomes an additional bounded proof surface layered on top of those
settled resident, decision, and realization surfaces.

## Retained Calc-Owned Host Surfaces

Calc intentionally remains host-owned in this cycle for:

- raw document mutation APIs
- all out-of-contract workbook and mutation classes
- the live document shell outside the admitted slice

Calc may still execute the rollback record in this cycle, but it must not
quietly reconstruct admitted rollback state with hidden local authority.

## Exact Success Standard

Final rollback reassessment counts as success only if all of the following
hold on the admitted slice:

- queue restore remains exact
- computational comparison returns a full match after rollback
- graph comparison remains exact and full-match after rollback
- admitted formula-cell lifetime, cell storage, wiring, formula-tree, and
  formula-track restore can be driven from an explicit engine-authored
  rollback surface
- rollback after induced divergence stays explicit and green

This cycle does not succeed merely because the live document looks usable
after rollback. The admitted restore proof lanes must remain exact.

## Immediate Hybrid Or Defer Triggers

This cycle must immediately keep rollback hybrid or defer broader rollback
migration if the proof surface requires any of the following:

- widening beyond the admitted scalar and narrow structural slice
- hidden Calc-only rollback state not representable by an engine-authored
  rollback record
- reintroducing host-owned after-state decisions inside rollback
- shared-group-sensitive, named-range-sensitive, off-sheet, or sheet-wide
  classes
- a fix that weakens exact queue, computational, graph, or replay
  verification

## Required Closeout Shape

This contract is satisfied only if the closeout ends in one of these explicit
states:

- engine-authored admitted-slice rollback proceeds
- admitted rollback remains hybrid with one clearly bounded reason
- broader rollback migration is deferred again with explicit reasons

Anything less explicit than that is out of contract.
