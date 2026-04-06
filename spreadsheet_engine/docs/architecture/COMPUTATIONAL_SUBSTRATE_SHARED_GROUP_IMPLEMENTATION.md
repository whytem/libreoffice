# Computational Substrate Shared-Group Implementation

Status: bounded structural preserve, split, and rebuild authority slice implemented

## What Landed

The shared-group widening cycle now has one bounded live structural authority
family on top of the ownership-complete admitted slice.

The workstream is now closed out by:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_WIDENING_DECISION_RECORD.md)

The implementation stays intentionally narrow:

- it widens the live admitted slice only for exact same-sheet shareable
  shared-group structural topology cases
- that exact family now includes bounded `Preserve`, `Split`, and `Rebuild`
  outcomes
- it keeps non-exact, named-range-sensitive, off-sheet, regrouping, and
  repair-sensitive shared-group classes outside live admission

## Main Runtime Surfaces

The landed path is centered in:

- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)
- [StructuralPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/StructuralPilotBuilder.hxx)
- [ComputationalSubstrateStructural.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateStructural.hxx)
- [ComputationalSubstrateRollout.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx)

## Facade-Side Shared-Group Surfaces

The facade consumer layer now exposes stable shared-group helpers for:

- collection of unique `FormulaGroupDescriptor` values through the workbook
  facade
- explicit transition classification between before and after group
  descriptors

That means tests and evidence can now describe shared-group outcomes as:

- `Preserve`
- `Rebuild`
- `Split`
- or `None`

without relying on hidden Calc-local interpretation.

## Structural Shared-Group Gate

The structural pilot recognizes a dedicated shared-group slice when all of
the following hold:

- the mutation stays inside the already-admitted structural vocabulary
- the workbook has no named-range-sensitive structural behavior
- the shared-group behavior stays single-sheet and shareable on the mutated
  sheet
- the shape stays inside ordinary-formula plus shared-group-member cells
- either the before or after shadow carries shared-group identity

This slice is guarded by:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL=1`
- `SPREADSHEET_ENGINE_COMPUTATIONAL_SHARED_GROUP=1`

Within that gate, the behavior now splits cleanly:

- validation mode still exercises bounded non-exact, regrouping, and
  repair-sensitive shared-group outcomes
- authority mode admits exact same-sheet shareable structural topology
  candidates

## Engine-Predicted Structural Topology

The structural predictor no longer stops at preserve-only group shifting.

For the admitted shared-group structural slice, it now:

- shifts each before-state shared-group member through the structural
  mutation independently
- drops members removed by the mutation
- partitions the surviving shifted members into explicit contiguous vertical
  runs
- materializes only runs longer than one cell as rebuilt shared groups
- leaves single surviving formulas unshared

That one engine-authored rule family naturally covers:

- `Preserve`: one contiguous shifted run with the original length
- `Split`: fewer or no surviving shared runs after the mutation
- `Rebuild`: one or more surviving shared runs whose anchor or length changes

## Validation Fallback Boundary

The host-observed shared-group topology overlay still exists, but only as a
bounded validation fallback.

It is now used only when:

- the shared-group slice is running in validation mode
- the engine-predicted shared-group topology does not exactly match the
  observed after-state

That keeps non-exact regrouping and repair-sensitive classes visible to the
pilot without relabeling them as admitted live authority.

## Authority Carry-Through

The admitted shared-group structural slice now carries through the full
authority-shaped after-state rather than stopping at candidate
classification.

The landed path:

- overlays live observed cell and formula payloads by address while keeping
  shared-group identity engine-authored
- rehydrates per-cell formula-tree and formula-track flags from the predicted
  observation state
- builds IR after-state group bindings from the predicted computational
  shadow, not from observed shared-group topology
- keeps the admitted after-shadow materializable into the mutable substrate
  facade without stale pre-shift formula source text

That is why the preserve, split, and rebuild slices can now close exact
queue, computational, graph, and IR checks on the live narrow rollout path.

## Test Coverage Added

The checked-in coverage now includes:

- facade consumer coverage for shifted structural `Preserve`, `Split`, and
  `Rebuild` classification
- standalone proof that exact structural split prediction does not depend on
  observed shared-group topology
- standalone proof that exact structural rebuild prediction does not depend
  on observed shared-group topology
- standalone proof that exact structural preserve, split, and rebuild
  authority candidates close exact computational, graph, and IR state
- structural shared-group rejection when the dedicated candidate gate is off
- structural shared-group preserve, split, and rebuild live-apply coverage
  when the dedicated shared-group gate is on
- structural shared-group-plus-named-range defer coverage
- structural shared-group repair-detected coverage when the group shape is
  perturbed after the structural mutation

The result is one bounded admitted structural family, not a broad
shared-group rollout.

The next adjacent reassessment is now captured in:

- [COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_ADMISSION_PLAN.md)
