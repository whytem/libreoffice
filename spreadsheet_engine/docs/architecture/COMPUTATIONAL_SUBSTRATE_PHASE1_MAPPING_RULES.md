# Computational Substrate Phase 1 Mapping Rules

Status: active Phase 1 mapping artifact

## Purpose

This document records how live Calc state maps into the Phase 1 computational
shadow.

The goal is to make the identity model explicit before Phase 1 mutation and
decision work widens further.

## Primary Ids

Phase 1 uses these durable engine-owned ids:

- cell identity: `api::CellAddress`
- formula-group identity: `(anchor address, group length)`
- listener-anchor identity: `(kind, anchor address, length)`
- broadcaster identity:
  - single-cell broadcaster: `api::CellAddress`
  - area broadcaster: `api::CellRange`

The mapping helpers are defined in
[ComputationalShadowMapping.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ComputationalShadowMapping.hxx).

## Mapping Rules

### Cells

- every non-empty facade cell becomes exactly one `ShadowCellRecord`
- the record id is the cell address
- scalar and formula cells share the same id model

### Formula Groups

- grouped formulas map by anchor address plus length
- member rows do not introduce independent group ids
- length is part of the id so a rebuild cannot silently treat a changed group
  extent as the same group

### Listener Anchors

- listener anchors come only from normalized observation capture
- formula-cell listeners map to `(FormulaCell, address, 1)`
- formula-group listeners map to `(FormulaGroup, anchor address, group length)`
- unknown listeners map to `(HostUnknown, normalized anchor, normalized length)`

### Named Ranges

- named-range descriptors remain facade-owned inputs into the shadow
- the shadow stores normalized descriptors, not Calc `ScRangeData*`

## Allowed Ephemeral Calc-Side Aids

Phase 1 allows transient Calc-side aids during rebuild only:

- cell address lookups needed to read live document content
- sheet-local and global named-range lookup during descriptor discovery
- live broadcaster capture used to produce normalized listener anchors

These aids must not become durable shadow identity.

## Forbidden Identity Shortcuts

Phase 1 explicitly forbids storing any of the following as shadow identity:

- `ScDocument*`
- `ScTable*`
- `ScColumn*`
- `ScFormulaCell*`
- `ColumnBlockPositionSet*`
- BASM slot or broadcaster-container pointers
- raw token-array addresses

## Resulting Boundary

The final stored shadow state is intentionally address- and descriptor-based.

That gives Phase 1 a stable comparison model while keeping Calc pointer
ownership and container layout outside the engine-owned shadow.
