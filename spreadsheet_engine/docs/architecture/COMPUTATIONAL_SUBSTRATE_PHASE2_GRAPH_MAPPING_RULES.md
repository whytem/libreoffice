# Computational Substrate Phase 2 Graph Mapping Rules

Status: active Phase 2 mapping and equivalence artifact

## Purpose

This note records the identity and equivalence rules for the Phase 2
dependency-graph shadow.

The corresponding helper surface is
[DependencyGraphShadowMapping.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/DependencyGraphShadowMapping.hxx).

## Primary Identities

Phase 2 uses these engine-owned ids:

- formula cell node: `ShadowCellId`
- formula group node: `ShadowFormulaGroupId`
- listener anchor: `ListenerAnchorId`
- broadcaster node: `BroadcasterNodeId`
- edge: `GraphEdgeRecord` using broadcaster id plus listener-anchor id

No Phase 2 graph identity may depend on:

- `ScFormulaCell*`
- `sc::FormulaGroupAreaListener*`
- `SvtBroadcaster*`
- BASM slot or bucket position
- broadcaster-store iterator position
- listener-context bookkeeping address

## Mapping Rules

The normalized mapping is:

- formula cell address -> `ShadowCellId`
- formula cell address -> formula-cell listener anchor with length `1`
- formula group anchor and length -> `ShadowFormulaGroupId`
- formula group anchor and length -> formula-group listener anchor
- cell broadcaster address -> `BroadcasterNodeId::forCell(...)`
- area broadcaster range -> `BroadcasterNodeId::forArea(...)`

Observed listeners that do not map back to an admitted formula cell or formula
group may still appear as listener-anchor records, but they remain
unresolved-by-design in the graph shadow. That keeps host-owned residue visible
instead of silently folding it into engine-owned identity.

## Normalized-Equivalent Cases

Phase 2 allows normalized equivalence for:

- formula-tree and formula-track subset order when membership is identical
- deduplicated listener-anchor collections
- deduplicated edge collections
- graph rebuilds that preserve semantic node and edge identity while live Calc
  containers churn internally

Phase 2 does not allow normalized equivalence to hide:

- node population mismatches
- broadcaster/listener anchor kind changes
- address or range changes
- formula-group anchor or length changes

## Allowed Ephemeral Calc Aids

Calc-owned observations may still be used transiently to build the graph:

- live broadcaster snapshots
- live listener snapshots
- formula-tree / formula-track captures
- temporary lookup through the workbook facade or computational shadow

Those aids are permitted only as build inputs. They are not durable graph
identity.
