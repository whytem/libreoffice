# Computational Substrate Cell Storage Implementation

Status: completed implementation note

## Purpose

This note records the first engine-owned admitted-slice cell-store
implementation for
[COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md).

The implementation goal for this workstream is narrower than full storage
migration:

- introduce an explicit engine-owned resident cell store
- bootstrap it from engine-owned computational shadow state
- keep it synchronized from engine-authored after-shadows for admitted
  authority, lifecycle, and structural transitions
- prove that it stays exact on the admitted slice before any Calc mirroring
  path is introduced

## Landed Engine-Owned Storage Shape

The resident store now lives in
[MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx).

The landed surface is:

- `AdmittedCellStorageRecord`
- `AdmittedCellStorage`
- `AdmittedCellStorageComparison`
- `buildAdmittedCellStorage(...)`
- `compareAdmittedCellStorage(...)`

The mutable substrate state now carries:

- `maCellStorage`

alongside the previously landed:

- `maFacade`
- `maObservation`
- `maShadow`

This makes resident cell storage explicit rather than treating the in-memory
facade as the only durable storage representation.

## Synchronization Model

The resident cell store now advances from engine-owned after-shadows, not
from rereading Calc after each mutation.

The landed synchronization model is:

- bootstrap from the initial computational shadow
- reconcile after admitted authority transitions
- reconcile after admitted lifecycle transitions
- reconcile after admitted structural transitions

The reconciliation path is intentionally value-semantic:

- it derives sorted admitted cell records from the engine-owned after-shadow
- it merges those records into the resident store
- it updates resident workbook generation from the engine-owned snapshot

This is still a bounded admitted-slice implementation, but it is no longer
just implicit facade state.

## Validation Surface

The landed proof lanes are:

- standalone resident-store checks in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- Calc admitted-slice checks in
  [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

The new checks verify:

- bootstrap exactness between resident cell storage and the admitted shadow
- exact resident-store updates after admitted scalar authority transitions
- exact resident-store updates after admitted lifecycle insert and remove
  transitions
- exact resident-store updates after admitted structural transitions on the
  existing bounded slice

## Boundary Kept Intact

This implementation still does not claim:

- Calc mirroring from engine-resident cells
- formula-cell object lifetime migration
- live broadcaster/listener container residency migration
- broader storage migration outside the admitted slice

Those remain later workstreams in the same plan.
