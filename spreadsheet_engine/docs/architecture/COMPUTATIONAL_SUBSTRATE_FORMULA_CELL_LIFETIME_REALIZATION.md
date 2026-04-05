# Computational Substrate Formula-Cell Lifetime Realization

Status: completed implementation note

## Purpose

This note records the Calc-side realization path for engine-owned admitted
formula-cell lifetime from
[COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md).

The goal of this workstream is intentionally narrow:

- keep engine-owned admitted formula-cell lifetime as the source of truth
- realize that resident lifetime state into live `ScFormulaCell` objects
- keep resident scalar cell storage and resident wiring realization as
  separate admitted responsibilities
- keep Calc as the temporary mutation-entry and rollback host

## Landed Realization Surface

The new compat realization surface lives in
[ComputationalSubstrateFormulaCellLifetime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateFormulaCellLifetime.hxx).

The landed public entry point is:

- `realizeAdmittedFormulaCellLifetime(...)`

The realization result classifies only two outcomes on this admitted slice:

- `Applied`
- `RejectedOutOfContract`

That keeps this pass honest. The helper either realizes the admitted lifetime
state exactly, or it rejects the workbook shape without silently reclaiming
authority in Calc.

## Realization Model

The landed realization model is:

- collect the currently live admitted formula-cell addresses from Calc
- reject any live formula cell already outside the admitted scalar contract
- remove live admitted formula cells that are no longer present in the
  engine-owned lifetime store
- realize or refresh admitted formula cells from resident lifetime records
- leave scalar cell replay to the existing resident cell-storage mirror
- leave listener, broadcaster, formula-tree, and formula-track realization to
  the existing resident wiring path

This deliberately separates three concerns that were previously easier to blur
together:

- formula-cell object lifetime
- scalar cell value residency
- live dependency container realization

## Supporting Split In Resident Cell Mirroring

To keep formula-cell lifetime authority separate from scalar cell replay, the
resident cell-storage compat surface now provides:

- `mirrorAdmittedCellStorage(...)`
- `mirrorAdmittedScalarCellStorage(...)`

The new scalar-only mirror path replays admitted scalar values without
recreating formula cells. That lets formula-cell object creation, refresh, and
removal stay driven by engine-owned lifetime state instead of leaking back
through resident cell storage.

## Validation Surface

The Calc-side proof lane is in
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx).

That lane now verifies:

- lifecycle insert realization from resident lifetime state
- lifecycle remove realization from resident lifetime state
- structural relocation realization from resident lifetime state
- exact computational and graph agreement after lifetime realization,
  scalar-cell replay, recalculation, and live wiring realization

## Boundary Kept Intact

This realization path still does not claim:

- direct engine-owned mutation entry
- broad formula-cell object lifetime outside the admitted slice
- shared-group-sensitive lifetime behavior
- named-range-sensitive lifetime widening
- host rollback migration

Those remain later proof surfaces.
