# Computational Substrate Phase 2 Graph Schema

Status: active Phase 2 schema artifact

## Purpose

This note records the engine-owned graph-shadow types introduced for Phase 2 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

The goal of the schema is to let the engine describe the admitted live
dependency graph subset without treating Calc listener objects, broadcaster
stores, BASM slots, or container pointers as durable graph identity.

## Ownership Rules

The Phase 2 graph schema is defined in
[DependencyGraphShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/DependencyGraphShadow.hxx).

It uses the following identity rules:

- formula-node identity is the normalized `ShadowCellId`
- formula-group identity is the normalized `ShadowFormulaGroupId`
- listener-anchor identity is the normalized `ListenerAnchorId`
- broadcaster identity is the normalized `BroadcasterNodeId`
- edge identity is the ordered pair of broadcaster id and listener-anchor id

The schema may use Calc observation as input, but it must not persist:

- `ScFormulaCell*`
- `sc::FormulaGroupAreaListener*`
- `SvtBroadcaster*`
- BASM slot addresses
- `ScBroadcastArea*`
- broadcaster-store or listener-context container addresses

## Core Types

The current Phase 2 schema contains:

- `GraphFormulaNodeRecord`
- `GraphFormulaGroupNodeRecord`
- `GraphListenerAnchorRecord`
- `GraphBroadcasterNodeRecord`
- `GraphEdgeRecord`
- `DependencyGraphShadow`

`DependencyGraphShadow` also carries the workbook snapshot and grammar so later
authority phases can compare graph state against the same workbook generation
that produced the shadow.

## Normalized Semantics

The graph schema is intentionally narrower than Calc's live storage model:

- formula cells and formula groups are modeled as graph-facing semantic nodes
- listener anchors are modeled independently from Calc listener objects
- broadcasters are modeled as normalized cell or area ids
- empty or delayed containers may still appear as broadcaster nodes if the live
  observed state has not yet purged them

This means Phase 2 comparisons can later distinguish:

- exact graph agreement
- normalized-equivalent graph agreement
- mismatched graph state

without reintroducing Calc container identity as the engine's truth model.
