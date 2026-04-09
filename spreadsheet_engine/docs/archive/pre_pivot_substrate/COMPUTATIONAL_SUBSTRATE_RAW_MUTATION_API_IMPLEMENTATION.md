# Computational Substrate Raw Mutation API Implementation

Status: implemented raw-mutation path

## Purpose

This note records the engine-authored raw mutation path added for
[COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md).

The implementation goal for this workstream is intentionally narrow:

- introduce one explicit engine-authored raw mutation record for the
  admitted slice
- have Calc consume that record instead of reconstructing admitted mutation
  intent inline at the mutation-entry call site
- keep realization and rollback in the existing admitted host shell for this
  cycle

## Landed Engine-Authored Surface

The new compat surface lives in:

- [ComputationalSubstrateRawMutation.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRawMutation.hxx)

It now defines:

- `RawMutationRecordKind`
- `AdmittedRawMutationRecord`
- `RawMutationRecordResultKind`
- `RawMutationRecordResult`
- `RawMutationApplyResultKind`
- `RawMutationApplyResult`
- `buildAdmittedRawMutationRecord(...)`
- `applyAdmittedRawMutationRecord(...)`

The admitted raw mutation record is value-semantic and carries only the
bounded identity needed by the admitted slice:

- mutation kind
- anchor address
- structural count when relevant
- explicit scalar payload when relevant
- explicit formula source when relevant
- optional formula cached value linkage already present on the admitted
  mutation-entry request

## Raw Mutation Flow

The landed raw mutation path is:

1. build an `AdmittedRawMutationRecord` from the admitted
   `MutationEntryRequest`
2. reject immediately if the request cannot be represented by the admitted
   record shape
3. apply that record into Calc through one compat surface instead of
   reinterpreting the request inline
4. observe the live after-state and continue through the existing admitted
   mutation-entry transition, resident-state application, realization, exact
   verification, and rollback path

The important change is that the live host shell no longer receives raw
mutation identity only as the original request shape. It now consumes one
explicit admitted record built by the engine-owned compat layer first.

## Mutation-Entry Integration

The admitted mutation-entry path in
[ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
now:

- builds the admitted raw mutation record before any live apply
- stores that record in `moRawMutationRecord`
- applies the record through `applyAdmittedRawMutationRecord(...)`
- records `moRawMutationObservation` for both applied and rolled-back paths

That makes raw mutation entry visible as its own admitted proof surface
instead of remaining an implicit prelude to realization and rollback.

## Boundary Kept Intact

This workstream still does not claim:

- broad `ScDocument` mutation API replacement
- shared-group, named-range-sensitive, off-sheet, or sheet-wide raw mutation
  support
- realization migration out of Calc
- rollback migration out of Calc

Calc remains the temporary live apply host for the admitted slice. The
landed change is that admitted raw mutation identity is now more explicitly
engine-authored before that host apply step executes.
