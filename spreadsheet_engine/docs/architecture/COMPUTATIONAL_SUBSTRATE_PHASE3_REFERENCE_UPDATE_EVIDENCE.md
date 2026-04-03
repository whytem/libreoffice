# Computational Substrate Phase 3 Reference Update Evidence

Status: active Phase 3 evidence note

## Purpose

This note records the admitted reference-shape and representative structural
update semantics for the first engine-owned execution IR slice.

The implementation lives in
[ExecutionIrReferenceUpdate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ExecutionIrReferenceUpdate.hxx).

## Admitted Reference Carriers

Phase 3 structural-update semantics are admitted for:

- single references
- range references
- column/row-name references
- external single references
- external range references

The update layer intentionally works on the IR payloads, not on Calc token
containers.

## Preserved Rules

The admitted update layer preserves:

- relative versus absolute reference flags
- deleted-flag propagation through the existing engine reference-data helpers
- range ordering and normalization through the engine reference-update APIs
- external-reference file and tab identity while updating only the embedded
  reference payload

## Structural Cases Explicitly Carried Forward

Phase 3 continues the representative structural cases already admitted by
Phase 2:

- single row insert
- single column delete

The update-plan builder also names the corresponding row-delete and
column-insert shapes, but Phase 3 evidence remains anchored to the carried
representative row-insert and column-delete cases.

## Explicit Defers

This note does not claim admitted semantics for:

- copy or move
- clipboard or load-time adjustments
- listener or broadcaster updates
- jump-table or matrix structural rewriting
- external-reference cache ownership

Those remain deferred until a later phase explicitly admits them.
