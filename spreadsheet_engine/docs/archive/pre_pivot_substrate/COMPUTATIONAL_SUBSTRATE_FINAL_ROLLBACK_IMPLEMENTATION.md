# Computational Substrate Final Rollback Implementation

Status: engine-authored admitted rollback path implemented

## Purpose

This note records the bounded implementation shape for
[COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md).

The goal of this implementation step is not broad mutation-shell migration.
It is to make admitted-slice rollback explicit and engine-authored instead of
quietly rebuilding before-state from Calc-local authority at the moment of
rollback.

## Landed Rollback Surface

The admitted rollback path now has one explicit compat surface:

- [ComputationalSubstrateRollback.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx)

That surface now owns:

- the admitted rollback record shape
- rollback queue comparison against the captured before-state formula queue
- rollback observation classification
- the engine-authored rollback application path

## Engine-Authored Rollback Record

The admitted rollback record is frozen as:

- resident object-realization payload for the before-state
- captured formula queue state for the before-state
- one admitted generation marker for the rollback record

This keeps rollback authority aligned with the resident and realization
surfaces already owned by the engine on the admitted slice:

- resident cell storage
- resident wiring containers
- formula-cell lifetime decisions
- mutation-entry decisions
- live object-realization records

## Calc Integration Shape

Calc still hosts rollback execution, but it no longer needs to reconstruct
rollback state ad hoc from local after-state assumptions.

The mutation-entry path now:

- builds an admitted rollback record up front from the captured before-state
- reuses that same rollback record for dirty-baseline rejection, verification
  failure, and repair-detected rollback
- records explicit rollback observation after rollback is executed

The mutation-entry landing point is:

- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)

## Bounded Runtime Proof

The runtime proof for this step covers:

- helper-level exact rollback restore from an explicit rollback record
- helper-level missing-restored-object diagnosis from the explicit rollback
  record path
- mutation-entry dirty-baseline rejection proving runtime rollback now
  produces explicit exact rollback observation

## What Still Remains Host-Owned

This implementation does not move:

- raw document mutation APIs
- the broader live document shell
- out-of-contract rollback classes
- workbook-wide rollback behavior outside the admitted slice

Calc still executes the rollback shell in this cycle. The change is that the
rollback shape it executes is now explicitly engine-authored.
