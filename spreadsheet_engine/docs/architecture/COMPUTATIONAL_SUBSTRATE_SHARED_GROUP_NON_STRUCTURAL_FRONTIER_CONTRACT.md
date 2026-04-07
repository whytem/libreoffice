# Computational Substrate Shared-Group Non-Structural Frontier Contract

Status: frozen contract for the broader non-structural shared-group frontier closeout

## Purpose

This note freezes the exact boundary used for the closeout of
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_FRONTIER_ADMISSION_PLAN.md).

The cycle began from the already-admitted non-structural member-exit slice
and asked whether the broader frontier could move into the admitted rollout.

The contract outcome is partial by design:

- one additional exact family now admits
- the rest of the frontier stays explicitly outside live admission

## Newly Admitted Family

This cycle admits one additional shared-group non-structural family:

- same-sheet shareable shared-group same-text preserve `SetFormula`

That family is bounded by all of the following:

- the touched address already belongs to a shareable shared group before the
  mutation
- the mutation is `SetFormula`
- the replacement formula text is byte-for-byte identical to the touched
  cell's pre-mutation formula source
- the observed after-state keeps the touched address inside the same
  shareable group identity
- named-range descriptors stay unchanged
- clean-baseline requirements from the existing admitted slice still hold

This family admits only through the same non-structural shared-group gate
already used by the member-exit slice:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP_NON_STRUCTURAL=1`

The live carry-through remains lane-specific:

- lifecycle for direct `SetFormula`
- mutation entry when the mutation-entry gate is also enabled

## Carried-Forward Admitted Family

The prior admitted member-exit family remains unchanged:

- same-sheet shareable shared-group member-exit `SetScalarValue`
- same-sheet shareable shared-group member-exit `SetFormula`
- same-sheet shareable shared-group member-exit `ClearCell`

This frontier cycle does not weaken or reinterpret that earlier boundary.

## Deferred Families

The following broader frontier families remain outside live admission:

- regroup after replacement formula changes the post-edit group identity
- merge after formula insertion or replacement collapses adjacent runs or
  prior groups
- named-range-combined shared-group non-structural behavior
- repair-sensitive host-only normalization classes
- off-sheet shared-group consumer or workbook-surface classes

These families may still be observable in tests or live Calc behavior, but
they are not admitted authority.

## Explicit Reject Rules

The closeout contract requires the runtime to reject these frontier shapes
instead of letting them pass as ordinary formula edits:

- formula insertion into a blank cell adjacent to a shareable shared group
- same-sheet grouped formula replacement that changes the touched cell's
  formula text and leaves the touched cell shared afterward
- grouped formula replacement that changes group identity across anchors or
  run boundaries
- named-range-combined shared-group frontier candidates

## Forbidden Shortcuts

The following remain forbidden for any newly admitted family:

- copying host-observed after-topology into the admitted after-state
- treating adjacent-to-shared-group formula insertion as an ordinary
  ungrouped insert
- inferring regroup or merge admission from normalized live verification
- widening named-range or off-sheet behavior from same-sheet preserve proof

## Closeout Outcome Categories

This cycle uses three outcome categories:

- admitted: same-text preserve `SetFormula`
- admitted from prior cycle: member-exit scalar/formula/clear
- deferred: regroup, merge, named-range-combined, repair-sensitive, and
  off-sheet classes
