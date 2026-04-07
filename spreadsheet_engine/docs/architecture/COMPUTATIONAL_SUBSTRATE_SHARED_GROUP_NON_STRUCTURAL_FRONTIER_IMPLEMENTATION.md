# Computational Substrate Shared-Group Non-Structural Frontier Implementation

Status: broader non-structural shared-group frontier closeout implemented

## What Landed

This cycle adds one new admitted family on top of the prior member-exit
slice:

- same-sheet shareable shared-group same-text preserve `SetFormula`

The landed runtime keeps that widening narrow:

- only already-shared touched addresses are eligible
- only identical formula-text replacement is newly admitted
- the touched address must stay in the same shareable group identity
- member-exit logic from the prior cycle remains unchanged

## Runtime Changes

The main changes landed in:

- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
- [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)

The closeout behavior is:

- the non-structural shared-group candidate gate now requires the touched
  cell to be shared before the mutation
- `SetFormula` admits only two shapes:
  - prior-cycle member exit when the touched cell is no longer shared after
  - same-text preserve when the touched cell stays in the same shareable
    group identity
- the rebuild window is bounded to the touched pre-existing group rather
  than neighboring formulas outside that group
- formula insertion into a blank cell adjacent to a shareable group now
  rejects as deferred shared-group frontier behavior instead of silently
  flowing through the ordinary lifecycle path

## Carry-Through Shape

The same-text preserve family uses the existing engine-authored carry-through
path:

- materialize the predicted after-shadow
- build the dependency snapshot from that predicted facade
- reapply observation state from the predicted dependency/recalc result
- compare against observed after-state for exact verification

That means same-text preserve now closes through the same exact
computational, graph, and IR pipeline already used by the admitted
member-exit family.

## What Did Not Land

This implementation intentionally does not admit:

- regroup identity changes
- merge identity changes
- named-range-combined frontier behavior
- repair-sensitive host normalization
- off-sheet consumer widening

Those cases either reject earlier now or remain outside the exact admitted
mapping rules.
