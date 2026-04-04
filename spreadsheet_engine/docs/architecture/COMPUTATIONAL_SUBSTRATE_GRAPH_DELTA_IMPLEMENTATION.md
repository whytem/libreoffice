# Computational Substrate Graph Delta Implementation

Status: completed implementation note

## What Landed

The storage-and-wiring pilot now has an explicit engine-owned delta surface
for the admitted graph and wiring state.

The implementation lives in:

- [GraphWiringDelta.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/GraphWiringDelta.hxx)

## Delta Surface

The engine now emits explicit value-semantic deltas for:

- formula-tree membership
- formula-track membership
- broadcaster-node presence
- listener-edge presence
- recalc plan output for the same mutation

The delta surface is derived from engine-owned before/after graph shadows,
not from Calc-side listener traversal.

## Current Limits

This delta layer still follows the current admitted contract:

- ordinary scalar formulas only
- admitted scalar lifecycle mutations
- admitted single-sheet row/column structural mutations
- clean baseline only

It is intentionally not yet a broad listener-container migration surface.
The next phase uses these deltas to drive retained Calc host containers on
the same admitted slice.
