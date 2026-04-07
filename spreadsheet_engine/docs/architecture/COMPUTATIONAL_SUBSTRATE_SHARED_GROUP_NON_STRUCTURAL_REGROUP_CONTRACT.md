# Computational Substrate Shared-Group Non-Structural Regroup Contract

Status: frozen contract for the exact regroup closeout

## Purpose

This note freezes the exact boundary used to close
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_REGROUP_ADMISSION_PLAN.md).

The regroup cycle began from the already-admitted non-structural
member-exit plus same-text preserve slice and asked whether one bounded
regroup family could move into the admitted authority surface.

## Newly Admitted Family

This cycle admits one additional shared-group non-structural family:

- same-sheet shareable shared-group edge-regroup `SetFormula`

That family is bounded by all of the following:

- the touched address already belongs to a shareable shared group before the
  mutation
- the mutation is `SetFormula`
- the touched address remains shared after the mutation
- the post-edit touched-group identity differs from the pre-mutation
  touched-group identity
- the touched address is an edge member of the pre-mutation group:
  - anchor regroup
  - tail regroup
- the engine-authored regroup window is limited to:
  - the touched pre-mutation group
  - plus a contiguous adjacent ordinary-formula run on the touched edge side
- the regroup window may absorb ordinary formulas only
- the regroup window may not absorb a different prior shared group
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

## Deferred Families

The following regroup-adjacent families remain outside live admission:

- interior regroup that cannot be authored from the bounded edge window
- merge across prior shared groups
- blank-cell insertion into a gap adjacent to a shared group
- named-range-combined regroup
- repair-sensitive host-only normalization
- off-sheet shared-group regroup or consumer widening

## Explicit Reject Rules

The closeout contract requires the runtime to reject these shapes instead of
letting them pass as ordinary formula edits:

- regroup candidates that require absorbing another prior shared group
- regroup candidates that require inserting a formula into a blank gap
- same-sheet grouped formula replacement whose touched after-group still
  depends on host-observed topology outside the bounded regroup window
- named-range-combined regroup candidates

## Forbidden Shortcuts

The following remain forbidden for the newly admitted regroup family:

- copying host-observed after-topology into the admitted after-state
- treating merge as regroup because the final live shape happens to match
- widening off-sheet or named-range-sensitive behavior from the same-sheet
  edge-regroup proof
- inferring interior regroup admission from normalized live verification

