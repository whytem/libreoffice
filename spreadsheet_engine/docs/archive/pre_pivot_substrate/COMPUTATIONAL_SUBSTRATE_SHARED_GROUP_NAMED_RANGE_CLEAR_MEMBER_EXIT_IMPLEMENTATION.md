# Computational Substrate Shared-Group Named-Range Clear Member-Exit Implementation

Status: completed implementation note for bounded named-range-combined
`ClearCell` member-exit closeout

## What Changed

The runtime gate for bounded named-range-combined shared-group
`MemberExit` widened from scalar-plus-`SetFormula` to include `ClearCell`.

The landed runtime change is intentionally narrow:

- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
  now admits bounded named-range-combined `ClearCell` `MemberExit` on the
  `GlobalSingleAreaSameSheet` surface
- the same file now derives a bounded observation-build option from the
  live before/after shadows for this lane
- the authority observation builder suppresses only the named-range
  shared-group area-listener contribution for the exact surviving
  post-clear group
- [LifecyclePilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx)
  now threads the same bounded observation-build option through the
  lifecycle lane

## What Did Not Change

This closeout does not widen:

- broader named-range-combined regroup, merge, or collapse
- repair-sensitive host normalization
- off-sheet shared-group behavior

## Proof Added

The closeout adds:

- standalone exactness proof for bounded named-range-combined `ClearCell`
  member-exit
- live authority apply proof for the same lane
- live lifecycle apply proof for the same lane
- live mutation-entry apply proof for the same lane
