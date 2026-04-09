# Computational Substrate Primitive Realization And Rollback Implementation

Status: engine-authored primitive-realization-rollback path landed

## Purpose

This note records the implementation shape landed for
[COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md).

The goal of this phase was not to replace all Calc realization or rollback
behavior. It was to make the admitted primitive realization and rollback
shell explicit and engine-authored on the already-admitted slice.

## Landed Surface

The landed primitive realization surface is centered in
[ComputationalSubstrateObjectRealization.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateObjectRealization.hxx).

That surface now carries:

- an explicit admitted primitive realization record layered on top of the
  settled admitted object-realization record
- a dedicated primitive realization apply result
- a dedicated primitive realization observation
- a classifier that narrows downstream object-realization proof to the
  primitive realization shell

The landed primitive rollback surface is centered in
[ComputationalSubstrateRollback.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollback.hxx).

That surface now carries:

- an explicit admitted primitive rollback record layered on top of the
  settled admitted rollback record
- a dedicated primitive rollback apply result
- a dedicated primitive rollback observation
- a classifier that narrows downstream rollback proof to the primitive
  rollback shell

## Runtime Integration

The mutation-entry runtime path now builds and consumes the primitive
realization and rollback records through
[ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx).

The integration shape is:

- build admitted raw-mutation and raw-document-mutation records from the
  admitted mutation-entry request
- build admitted primitive rollback record from the before-state rollback
  record
- build admitted primitive realization record from the after-state
  object-realization record
- apply the primitive realization or primitive rollback record to the live
  Calc document
- continue through the already-settled queue, computational, graph,
  raw-mutation, raw-document-mutation, and live-apply proof surfaces
- store both object-realization or rollback observations and the new
  primitive realization or rollback observations on the mutation-entry
  result

This keeps primitive host execution explicit without reopening resident
storage, resident wiring, lifetime, raw mutation, raw document mutation, or
live-apply ownership.

## Bounded Scope

The landed path remains intentionally bounded to:

- `SetScalarValue`
- `SetFormula`
- `ClearCell`
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`

Anything outside that admitted slice still rejects as out of contract.

## Test Coverage

The landed implementation is covered in
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
through:

- dedicated primitive realization observation classifier coverage
- dedicated primitive rollback observation classifier coverage
- admitted mutation-entry success assertions that now require primitive
  realization exactness
- dirty-baseline rollback assertions that now require primitive rollback
  exactness

## Current Meaning

This implementation does not yet prove that Calc no longer owns all host
execution around realized or restored state. It does prove that the admitted
primitive realization and rollback shell is now:

- explicit
- value-semantic
- engine-authored before live execution
- observable independently from the broader object-realization and rollback
  identity layers
