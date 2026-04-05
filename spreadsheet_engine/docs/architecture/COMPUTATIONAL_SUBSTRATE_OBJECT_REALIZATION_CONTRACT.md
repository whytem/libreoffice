# Computational Substrate Object Realization Contract

Status: frozen object-realization contract

## Purpose

This note freezes the exact proof surface for
[COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md)
before any object-realization implementation work proceeds.

The purpose of this contract is to keep the next cycle tightly focused on one
remaining host-owned surface on the admitted slice:

- live admitted object realization
- not resident state ownership
- not mutation-entry routing
- not broad rollback migration

This is a realization reassessment, not a broad authority rewrite.

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

## Live Object Classes Under Reassessment

This cycle is limited to the admitted live realization classes that currently
sit between engine-owned resident state and the live Calc document model:

- admitted `ScFormulaCell` create, replace, and remove realization
- admitted listener and broadcaster realization from engine-owned resident
  wiring state
- admitted formula-tree live participation
- admitted formula-track live participation

The cycle may introduce an explicit engine-authored realization record for
that bounded surface, but it must not claim broad host independence outside
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

Object realization becomes an additional bounded proof surface layered on top
of those settled resident and decision surfaces.

## Retained Calc-Owned Host Surfaces

Calc intentionally remains host-owned in this cycle for:

- raw document mutation APIs
- final rollback after failed verification
- all out-of-contract workbook and mutation classes
- the live document shell outside the admitted slice

Calc may still host live realization mechanics, but it must not quietly
reclaim after-state authority on the admitted slice.

## Exact Success Standard

Object-realization reassessment counts as success only if all of the
following hold on the admitted slice:

- queue comparison remains exact
- computational comparison remains a full match
- graph comparison remains exact and full-match
- admitted formula-cell, wiring, formula-tree, and formula-track realization
  can be driven from an explicit engine-authored surface
- rollback remains explicit and green on induced divergence

This cycle does not succeed merely because the live document looks similar
after realization. The admitted proof lanes must remain exact.

## Immediate Defer Triggers

This cycle must immediately defer broader object-realization migration if the
proof surface requires any of the following:

- widening beyond the admitted scalar and narrow structural slice
- hidden Calc-only object identity not representable by engine-authored
  realization records
- rollback migration out of Calc
- shared-group-sensitive, named-range-sensitive, off-sheet, or sheet-wide
  classes
- a fix that weakens exact queue, computational, or graph verification

## Required Closeout Shape

This contract is satisfied only if the closeout ends in one of these explicit
states:

- engine-authored admitted-slice object realization proceeds
- admitted object realization remains validation-only or hybrid with one
  clearly bounded reason
- broader object-realization migration is deferred again with explicit
  reasons

Anything less explicit than that is out of contract.
