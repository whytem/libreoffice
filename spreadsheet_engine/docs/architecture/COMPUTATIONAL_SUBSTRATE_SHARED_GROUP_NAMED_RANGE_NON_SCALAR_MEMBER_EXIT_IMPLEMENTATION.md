# Computational Substrate Shared-Group Named-Range Non-Scalar Member-Exit Implementation

Status: completed implementation note for bounded named-range-combined
non-scalar member-exit closeout

## What Changed

The runtime gate for bounded named-range-combined shared-group
`MemberExit` widened from scalar-only to scalar-plus-`SetFormula`.

The landed runtime change is intentionally narrow:

- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
  now admits named-range-combined `MemberExit` for `SetFormula` on the
  bounded `GlobalSingleAreaSameSheet` surface
- the same shared-group prediction and verification path already used by
  the named-range preserve and scalar member-exit lanes now carries the
  bounded `SetFormula` lane

## What Did Not Change

The adjacent bounded `ClearCell` lane was evaluated and then left deferred.

The exploratory authority-side cleanup experiment was removed because it did
not address the real blocker. The remaining mismatch is planner-side graph
prediction, not live broadcaster cleanup.

## Proof Added

The closeout adds:

- a standalone exactness proof for bounded named-range-combined
  `SetFormula` member-exit
- live authority apply proof for the same lane
- live lifecycle apply proof for the same lane
- live mutation-entry apply proof for the same lane
- retained reject proof for bounded named-range-combined `ClearCell`
  member-exit
