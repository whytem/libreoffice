# Computational Substrate Raw Document Mutation API Implementation

Status: engine-authored raw-document-mutation path landed

## Purpose

This note records the implementation shape landed for
[COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md).

The goal of this phase was not to replace all Calc mutation execution. It
was to make the admitted primitive document-mutation shell explicit and
engine-authored on the already-admitted slice.

## Landed Surface

The landed raw document mutation surface is centered in
[ComputationalSubstrateRawMutation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx).

That surface now carries:

- an explicit admitted raw document mutation record layered on top of the
  settled admitted raw-mutation record
- a dedicated raw document mutation apply result
- a dedicated raw document mutation observation
- a classifier that narrows downstream raw-mutation proof to the primitive
  document-mutation shell

## Runtime Integration

The mutation-entry runtime path now builds and consumes the primitive
document-mutation record through
[ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx).

The integration shape is:

- build admitted raw-mutation record from the admitted mutation-entry
  request
- build admitted raw document mutation record from that settled raw-mutation
  identity
- apply the primitive document mutation record to the live Calc document
- continue through the already-settled realization, verification, rollback,
  and live-apply surfaces
- store both raw-mutation and raw-document-mutation observations on the
  mutation-entry result

This keeps primitive execution explicit without reopening resident storage,
wiring, lifetime, realization, rollback, or live-apply ownership.

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

- dedicated raw document mutation observation classifier coverage
- admitted mutation-entry success assertions that now require both raw
  mutation and raw document mutation exactness
- admitted dirty-baseline rollback assertions that confirm the primitive
  document-mutation record and observation are present on the bounded path

## Current Meaning

This implementation does not yet prove that Calc no longer owns all
primitive document mutation behavior. It does prove that the admitted
primitive mutation shell is now:

- explicit
- value-semantic
- engine-authored before live execution
- observable independently from the broader raw-mutation identity layer
