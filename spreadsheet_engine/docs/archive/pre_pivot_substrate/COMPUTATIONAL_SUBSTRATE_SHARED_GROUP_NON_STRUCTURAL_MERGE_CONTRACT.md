# Computational Substrate Shared-Group Non-Structural Merge Contract

Status: frozen contract for the exact merge closeout

## Purpose

This note freezes the exact boundary used to close
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_ADMISSION_PLAN.md).

The merge cycle began from the already-admitted member-exit, same-text
preserve, and edge-regroup slice and asked whether one bounded merge family
could move into the admitted authority surface.

## Newly Admitted Family

This cycle admits one additional shared-group non-structural family:

- same-sheet shareable shared-group gap-closing merge `SetFormula`

That family is bounded by all of the following:

- the mutation is `SetFormula`
- the touched address is blank before the mutation
- the touched address becomes formula and shared after the mutation
- exactly two prior shareable same-column groups participate:
  - one immediately above the touched address
  - one immediately below the touched address
- the admitted merge window is exactly:
  - the upper prior group
  - the touched gap cell
  - the lower prior group
- the engine must author one exact merged after-group across that full
  contiguous window
- named-range descriptors stay unchanged
- clean-baseline requirements from the existing admitted slice still hold

This family admits only through the existing dedicated shared-group
non-structural gate:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP_NON_STRUCTURAL=1`

The live carry-through remains bounded:

- lifecycle for direct `SetFormula`
- mutation entry when the mutation-entry gate is also enabled

## Carried-Forward Admitted Families

The prior admitted non-structural shared-group families remain unchanged:

- same-sheet shareable shared-group member-exit `SetScalarValue`
- same-sheet shareable shared-group member-exit `SetFormula`
- same-sheet shareable shared-group member-exit `ClearCell`
- same-sheet shareable shared-group same-text preserve `SetFormula`
- same-sheet shareable shared-group edge-regroup `SetFormula`

## Deferred Families

The following merge-adjacent families remain outside live admission:

- one-sided adjacent formula insertion next to only one prior shared group
- replacement-driven merge that absorbs another prior shared group
- multi-group collapse beyond the bounded two-participant gap family
- named-range-combined merge
- repair-sensitive host-only normalization
- off-sheet shared-group merge or consumer widening
- non-edge regroup outside the already-admitted edge-regroup family

## Explicit Reject Rules

The closeout contract requires the runtime to reject these shapes instead of
letting them pass as ordinary formula edits:

- blank-cell insertion adjacent to only one prior shared group
- merge candidates that would need to absorb more than two prior
  participants
- replacement-driven merge whose authority still depends on host regrouping
- named-range-combined merge candidates

## Forbidden Shortcuts

The following remain forbidden for the newly admitted merge family:

- copying host-observed after-topology into the admitted after-state
- treating one-sided extension as admitted merge because live Calc grouped
  it
- inferring replacement-driven or multi-group merge admission from the
  bounded gap-closing proof
- widening named-range or off-sheet behavior from the same-sheet gap-merge
  proof
