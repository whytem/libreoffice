# Computational Substrate Shared-Group Implementation

Status: validation-only pilot implemented

## What Landed

The shared-group widening cycle now has a bounded validation-only pilot path
on top of the ownership-complete admitted slice.

The workstream is now closed out by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md)

The implementation stays intentionally narrow:

- it does not widen the live admitted slice
- it does not reopen already-settled ownership seams
- it adds only the minimum runtime support needed to exercise shared-group
  preserve, split, rebuild, and repair-sensitive structural cases through
  the existing structural validation lane

## Main Runtime Surfaces

The pilot is centered in:

- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)
- [StructuralPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/StructuralPilotBuilder.hxx)
- [ComputationalSubstrateStructural.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateStructural.hxx)
- [ComputationalSubstrateRollout.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx)

## Facade-Side Shared-Group Surfaces

The facade consumer layer now exposes two additional shared-group helpers:

- collection of unique `FormulaGroupDescriptor` values through the stable
  workbook facade
- explicit transition classification between before and after group
  descriptors

That means tests and evidence can now describe shared-group outcomes as:

- `Preserve`
- `Rebuild`
- `Split`
- or `None`

without relying on hidden Calc-local interpretation.

## Structural Validation Gate

The structural pilot now recognizes a dedicated validation-only shared-group
slice when all of the following hold:

- the mutation stays inside the already-admitted structural vocabulary
- the workbook has no named-range-sensitive structural behavior
- the shared-group behavior stays single-sheet and shareable on the mutated
  sheet
- the shape stays inside ordinary-formula plus shared-group-member cells
- either the before or after shadow carries shared-group identity

This slice is guarded by:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL=1`
- `SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP=1`

The shared-group gate does not widen the live admitted slice. It only
enables validation-only candidate execution.

## Validation-Only Topology Use

For the shared-group validation slice, the predicted structural population
uses the host-observed after-state shared-group topology for group identity
comparison.

That choice is deliberate:

- it allows the pilot to exercise bounded shared-group cases through the
  current structural validation seam
- it keeps preserve, split, rebuild, and repair-sensitive outcomes visible
- it does not relabel host-observed shared-group topology as admitted live
  authority

This is why the pilot is still validation-only in this workstream.

## Test Coverage Added

The Calc-side coverage now includes:

- facade consumer coverage for collected group descriptors and shifted
  preserve classification
- facade consumer coverage for same-shape `SetFormula` preserve and
  `ClearCell` split classification
- structural shared-group rejection when the dedicated candidate gate is off
- structural shared-group validation-only preserve candidate coverage when
  the gate is on
- structural shared-group-plus-named-range defer coverage
- structural shared-group repair-detected coverage when the group shape is
  perturbed after the structural mutation

These tests intentionally exercise the shared-group widening logic without
claiming live admission.
