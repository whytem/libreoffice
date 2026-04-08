# Computational Substrate Broader Off-Sheet Formula-Retained Carry-Through Decision Record

Status: completed closeout for the bounded broader off-sheet widening pass

## Decision

Admit the bounded direct one-consumer-sheet off-sheet shared-group
formula-retained `SetFormula` lane for:

- `SameTextPreserve`
- `Regroup`

Keep the following deferred:

- off-sheet named-range-combined families
- off-sheet merge, replacement-merge, and multi-group collapse
- multi-consumer-sheet and workbook-wide off-sheet authority

## Why

The previous off-sheet pass removed the raw-reference opacity blocker and
admitted direct off-sheet `MemberExit`.

This pass then proved that the adjacent direct off-sheet formula-retained
families already close exactly on the current runtime surface:

- the facade classifies the host shape distinctly
- standalone authority planning is exact
- live authority, lifecycle, and mutation-entry carry-through all apply
  exactly on the bounded lane

No new runtime repair was required for the direct off-sheet lane. The
existing admission logic was already sufficient once the cross-sheet raw
reference surface was no longer opaque.

## Evidence Summary

The pass added proof for:

- direct off-sheet `SameTextPreserve` classification
- direct off-sheet `Regroup` classification
- standalone exact authority planning for both families
- live authority carry-through for both families
- live lifecycle carry-through for both families
- live mutation-entry carry-through for both families

## Current Off-Sheet Boundary

The admitted off-sheet boundary now includes:

- one-consumer-sheet direct off-sheet `MemberExit`
- one-consumer-sheet direct off-sheet `SameTextPreserve`
- one-consumer-sheet direct off-sheet `Regroup`

The retained off-sheet boundary is now:

- off-sheet named-range-combined behavior
- broader off-sheet merge-shaped behavior
- multi-consumer-sheet and workbook-wide authority

## Next Logical Blocker

The next off-sheet blocker is no longer generic formula-retained carry-
through.

The next logical blocker is exact off-sheet named-range-combined boundary
surfacing and carry-through, followed by any broader off-sheet merge-shaped
families that remain real live candidates after that.
