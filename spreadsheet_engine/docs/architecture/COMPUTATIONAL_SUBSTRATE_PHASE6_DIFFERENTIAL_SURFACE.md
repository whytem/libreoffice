# Computational Substrate Phase 6 Differential Surface

Status: completed differential surface for Phase 6

## Purpose

This note records the explicit verdict categories exercised by the Phase 6
structural-authority lane.

It exists so the admitted structural pilot is not judged only by its happy
path. The closeout surface must show which cases apply cleanly, which cases
reject intentionally, and which cases trigger repair-detected rollback.

## Applied Structural Cases

The admitted live structural pilot currently exercises:

- `InsertRows`
  - ordinary scalar-formula slice only
  - exact queue, computational, and graph verification
- `DeleteColumns`
  - ordinary scalar-formula slice only
  - exact queue, computational, and graph verification

Checked-in Calc differential lanes:

- `testComputationalStructuralInsertRowPilot`
- `testComputationalStructuralDeleteColumnPilot`

## Normalized-Equivalent Structural Cases

Phase 6 does not currently admit any normalized-equivalent structural outcomes.

The structural pilot remains stricter than the Phase 5 scalar lifecycle lane:

- queue comparison is exact
- computational comparison is exact
- graph comparison is exact
- IR mismatch is treated as repair-detected on the admitted subset

## Rejected Structural Cases

The structural pilot rejects intentionally when:

- the captured baseline is dirty
- the mutation class is validation-only or out of contract

Checked-in Calc differential lanes:

- `testComputationalStructuralRejectsDirtyBaseline`
- `testComputationalStructuralRejectsValidationOnlyMutation`

## Repair-Detected Rollback Cases

The structural pilot treats silent post-mutation reference repair as
unacceptable on the admitted subset.

The current checked-in rollback lane is:

- `testComputationalStructuralRepairDetectedRollback`

That lane proves:

- the engine detects the mismatched admitted reference-update answer
- the result is classified as `RepairDetected`
- the bridge inverts the admitted structural edit
- the pre-mutation sheet slice is restored from the computational shadow

## Observed Verification Surface

Phase 6 currently treats execution-IR comparison as observation data, but the
structural bridge still requires the admitted row-insert and column-delete
reference updates to match before it will proceed.

This means the current structural differential surface is:

- exact queue comparison
- exact computational comparison
- exact graph comparison
- IR mismatch treated as repair-detected on the admitted subset
