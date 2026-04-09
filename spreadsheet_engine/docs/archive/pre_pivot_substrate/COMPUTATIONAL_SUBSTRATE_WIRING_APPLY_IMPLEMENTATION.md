# Computational Substrate Wiring Apply Implementation

Status: completed implementation note

## What Landed

The storage-and-wiring pilot now has a Calc-side apply layer for the admitted
live wiring surface.

The implementation lives in:

- [ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx)

## Apply Model

The apply layer is intentionally narrow and host-facing:

- it keeps Calc as the owner of `ScDocument` storage and live containers
- it clears the admitted live listener/tree/track state on the target
  document
- it rebuilds that state from the engine-owned target sets carried in the
  graph delta bundle

The current admitted host replay uses:

- `StartListeningCell`
- `StartListeningArea`
- `PutInFormulaTree`
- `AppendToFormulaTrack`

## Current Limits

The apply layer rejects anything outside the admitted formula shape:

- shared groups
- matrix formulas
- non-formula listener anchors

That matches the current storage-and-wiring contract. Broader container and
storage migration remains a later decision, not something this apply layer
claims today.
