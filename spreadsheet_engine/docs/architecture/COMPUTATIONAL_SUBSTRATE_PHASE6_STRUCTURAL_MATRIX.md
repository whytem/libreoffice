# Computational Substrate Phase 6 Structural Matrix

Status: active mutation matrix for Phase 6

## Purpose

This matrix records the admitted, validation-only, deferred, and rejected
structural mutation classes for Phase 6 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

It exists so later implementation and validation work cannot silently widen
the admitted live structural pilot.

## Admitted In Live Authority

- `InsertRows`
  - only on a single sheet
  - only on the clean-baseline scalar-formula subset
  - only when no shared-group or named-range structural repair is required
- `DeleteColumns`
  - only on a single sheet
  - only on the clean-baseline scalar-formula subset
  - only when no shared-group or named-range structural repair is required

## Validation-Only In Phase 6

- `DeleteRows`
- `InsertColumns`
- representative structural cases that touch named-range-sensitive formulas
  only in evidence lanes
- representative structural cases that prove deferred shared-group handling
  remains out of contract

## Explicitly Deferred

- shared-group creation, split, merge, and repair
- named-range structural authority in the live path
- sheet insert, delete, move, or rename authority
- external-reference structural authority
- copy, move, clipboard, load-time, and undo-like structural behavior
- broad range, union, and intersection repair outside what the admitted row
  insert and column delete cases require

## Explicitly Rejected In The Live Pilot

- scalar mutation classes already covered by earlier phases
- any structural mutation on a dirty baseline
- any structural mutation that requires retained Calc listener or broadcaster
  ownership in a way the engine cannot model directly
- any structural mutation that widens beyond ordinary scalar formulas
