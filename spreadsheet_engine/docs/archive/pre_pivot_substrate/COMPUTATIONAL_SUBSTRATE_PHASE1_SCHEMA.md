# Computational Substrate Phase 1 Schema

Status: active Phase 1 schema artifact

## Purpose

This document names the engine-owned shadow types introduced for Phase 1 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

The schema is intentionally narrow. It is the first engine-owned model of the
observed computational substrate subset proven in Phase 0.

## Ownership Rules

The Phase 1 shadow is owned entirely by `spreadsheet_engine`.

Its durable identity model is value-semantic:

- cell identity is `api::CellAddress`
- formula-group identity is anchor address plus group length
- listener-anchor identity is normalized anchor address plus kind and length
- broadcaster identity is normalized cell or area address

The Phase 1 shadow must not store Calc pointers as durable identity:

- no `ScDocument*`
- no `ScTable*`
- no `ScColumn*`
- no `ScFormulaCell*`
- no `ColumnBlockPositionSet*`
- no BASM slot pointers

Calc-owned addresses may still be used transiently during rebuild, but the
final stored shadow state must remain pointer-free.

## Named Types

The Phase 1 schema is defined in
[ComputationalShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadow.hxx).

The core types are:

- `ShadowCellId`
- `ShadowFormulaGroupId`
- `ListenerAnchorId`
- `ShadowCellRecord`
- `ShadowFormulaGroupRecord`
- `CellBroadcasterRecord`
- `AreaBroadcasterRecord`
- `ComputationalSheetShadow`
- `ComputationalWorkbookShadow`

## Representation Notes

`ComputationalWorkbookShadow` is a snapshot-style model. In Phase 1 it is
allowed to be fully rebuilt after supported mutations.

`ShadowCellRecord` is the central unit:

- every non-empty computation-facing cell appears once
- scalar and formula cells share the same record shape
- formula-bearing cells additionally carry normalized formula metadata,
  group linkage, and formula-tree / formula-track membership flags

This makes the shadow suitable for differential comparison against:

- workbook-facade cell state
- live formula-tree and formula-track capture
- normalized broadcaster and listener capture
- grouped-formula observations

## Explicit Defers

The Phase 1 schema does not attempt to model:

- BASM slot layout
- Calc token-array ownership
- broadcaster container internals
- authoritative listener lifecycle
- opaque Calc object identity

Those remain outside the Phase 1 shadow boundary.
