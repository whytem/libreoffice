# Computational Substrate Shared-Group Non-Structural Replacement-Merge Contract

Status: frozen contract for the exact replacement-merge closeout

## Purpose

This note freezes the exact boundary used to close
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REPLACEMENT_MERGE_ADMISSION_PLAN.md).

The replacement-merge cycle began from the already-admitted member-exit,
same-text preserve, edge-regroup, and gap-closing merge slice and asked
whether one additional true merge family could move into live admission.

## Newly Admitted Family

This cycle admits one additional shared-group non-structural family:

- same-sheet shareable shared-group edge replacement-merge `SetFormula`

That family is bounded by all of the following:

- the mutation is `SetFormula`
- the touched address is shared before the mutation
- the touched address is shared after the mutation
- the touched address is the anchor or tail edge of the touched pre-group
- exactly two prior shareable same-column groups participate:
  - the touched pre-group
  - one adjacent prior group on the touched edge side
- the admitted rebuild window is exactly:
  - the adjacent prior group
  - the full touched pre-group
- the engine must author an exact after-group that:
  - includes the touched address
  - fully absorbs the adjacent prior group
  - stays bounded within the admitted rebuild window
- untouched remainder from the touched pre-group may stay outside the
  merged after-group if the exact lowered rebuild yields that result
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
- same-sheet shareable shared-group gap-closing merge `SetFormula`

## Deferred Families

The following replacement-merge-adjacent families remain outside live
admission:

- one-sided adjacent formula insertion next to only one prior shared group
- multi-group collapse beyond the bounded two-participant family
- named-range-combined replacement merge
- repair-sensitive host-only normalization
- off-sheet shared-group replacement merge or consumer widening
- non-edge regroup or non-edge merge outside the admitted edge families

## Explicit Reject Rules

The closeout contract requires the runtime to reject these shapes instead of
letting them pass as ordinary formula edits:

- replacement candidates where the touched address is not on a touched-group
  edge
- replacement candidates that would need to absorb more than one adjacent
  prior shared group
- replacement candidates whose exact after-group would exclude the touched
  address
- one-sided adjacent insertion and named-range-combined cases

## Forbidden Shortcuts

The following remain forbidden for the newly admitted replacement-merge
family:

- copying host-observed after-topology into the admitted after-state
- relabeling a broader regroup or multi-group collapse as admitted
  replacement merge
- inferring named-range or off-sheet admission from the bounded same-sheet
  replacement proof
