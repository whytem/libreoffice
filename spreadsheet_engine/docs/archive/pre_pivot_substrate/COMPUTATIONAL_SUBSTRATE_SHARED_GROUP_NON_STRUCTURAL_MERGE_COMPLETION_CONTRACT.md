# Computational Substrate Shared-Group Non-Structural Merge Completion Contract

Status: frozen contract for the same-sheet merge-completion closeout

## Purpose

This note freezes the exact boundary used to close
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_MERGE_COMPLETION_PLAN.md).

The merge-completion cycle began from the already-admitted gap-closing and
edge replacement-merge families and asked whether the remaining same-sheet
single-adjacent insert class could move into the admitted slice.

## Newly Admitted Family

This cycle admits one additional shared-group non-structural family:

- same-sheet shareable shared-group one-sided adjacent insertion `SetFormula`

That family is bounded by all of the following:

- the mutation is `SetFormula`
- the touched address is blank before the mutation
- the touched address becomes formula and shared after the mutation
- exactly one adjacent shareable same-column prior group participates
- the opposite side of the touched address does not contribute another
  shareable prior group
- the inserted formula extends that prior group by exactly one cell
- the admitted rebuild window is exactly:
  - the inserted formula cell
  - the adjacent prior group
- the engine must author one exact after-group across that full window
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
- same-sheet shareable shared-group edge replacement-merge `SetFormula`

## Deferred Families

The following merge-adjacent families remain outside live admission:

- multi-group collapse beyond the current two-participant merge families
- named-range-combined merge
- repair-sensitive host-only normalization
- off-sheet merge widening
- broader non-edge regroup or merge

## Explicit Reject Rules

The closeout contract requires the runtime to reject these shapes instead of
letting them pass as ordinary formula inserts:

- blank-cell insertion adjacent to one prior shared group when the observed
  after-group is not exactly prior-length-plus-one
- insertions that would absorb another prior shared group or a broader
  multi-participant collapse
- named-range-combined merge candidates

## Forbidden Shortcuts

The following remain forbidden for the newly admitted family:

- copying host-observed after-topology into the admitted after-state
- treating broader merge collapse as admitted because live Calc grouped it
- inferring named-range or off-sheet admission from the bounded same-sheet
  one-sided proof
