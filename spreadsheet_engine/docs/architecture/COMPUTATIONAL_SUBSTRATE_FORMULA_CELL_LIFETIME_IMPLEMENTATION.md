# Computational Substrate Formula-Cell Lifetime Implementation

Status: completed implementation note

## Purpose

This note records the first engine-owned admitted-slice formula-cell lifetime
implementation for
[COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md).

The implementation goal for this workstream is narrower than broad object
ownership migration:

- introduce an explicit engine-owned admitted formula-cell lifetime store
- bootstrap it from the engine-owned computational shadow
- keep it synchronized from engine-authored after-shadows for admitted
  authority, lifecycle, and structural transitions
- prove that it stays exact on the admitted slice before any Calc realization
  path consumes it directly

## Landed Engine-Owned Lifetime Shape

The resident lifetime store now lives in
[MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx).

The landed surface is:

- `AdmittedFormulaCellLifetimeRecord`
- `AdmittedFormulaCellLifetime`
- `AdmittedFormulaCellLifetimeComparison`
- `buildAdmittedFormulaCellLifetime(...)`
- `compareAdmittedFormulaCellLifetime(...)`

The mutable substrate state now carries:

- `maFormulaCellLifetime`

alongside the already-proven:

- `maCellStorage`
- `maWiringContainers`
- `maShadow`
- `maGraphShadow`

This makes admitted formula-cell lifetime explicit rather than leaving it as
an implicit consequence of retained Calc object ownership.

## Synchronization Model

The resident lifetime store now advances from engine-owned after-shadows, not
from rereading Calc after each mutation.

The landed synchronization model is:

- bootstrap from the initial computational shadow
- reconcile after admitted authority transitions
- reconcile after admitted lifecycle transitions
- reconcile after admitted structural transitions

The reconciliation path is intentionally value-semantic:

- admitted formula-cell records are derived from ordinary scalar formula cells
  only
- records are sorted by normalized address
- replace semantics are captured through updated formula source text at the
  same admitted identity
- resident generation is updated from the engine-owned after-shadow

This is still a bounded admitted-slice implementation, but it is no longer
just implicit live host lifetime.

## Validation Surface

The landed proof lane is in
[computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx).

That lane now verifies:

- bootstrap exactness between resident formula-cell lifetime and the admitted
  initial computational shadow
- exact lifetime-store updates after admitted scalar authority transitions
- exact lifetime-store updates after admitted lifecycle insert and remove
  transitions
- exact lifetime-store updates after admitted structural transitions on the
  existing bounded slice

## Boundary Kept Intact

This implementation still does not claim:

- Calc realization directly from resident formula-cell lifetime
- direct engine-owned mutation entry
- broad formula-cell object lifetime migration outside the admitted slice
- shared-group-sensitive or named-range-sensitive lifetime behavior

Those remain later workstreams in the same plan.
