# Computational Substrate Cell Storage Mirroring

Status: completed implementation note

## Purpose

This note records the Calc-side mirroring path for the first
cell-storage-residency pilot in
[COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md).

The goal of this workstream is to let Calc refresh the admitted live cell
surface from engine-resident cell state without reclaiming authority for the
admitted after-state.

## Landed Mirroring Surface

The landed compat helper is:

- [ComputationalSubstrateCellStorage.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateCellStorage.hxx)

The main entry point is:

- `mirrorAdmittedCellStorage(...)`

It mirrors the admitted cell store into `ScDocument` while keeping the
already-proven graph and wiring replay path separate.

## Mirroring Model

The landed mirroring path works as follows:

1. collect the currently live admitted cell addresses from Calc
2. reject shared-group or matrix formula shapes immediately
3. clear live admitted cells that are no longer present in the engine store
4. materialize or refresh each admitted engine-owned cell record into Calc
5. let the existing engine-owned graph and wiring replay path rebuild the
   dependency-side host surface

This keeps the authority split explicit:

- engine-owned resident cell store decides the admitted cell after-state
- Calc mirrors that state into the retained host document model
- existing wiring replay still realizes the engine-owned dependency-side
  target state

## Supported Admitted Cell Classes

The landed mirroring path supports:

- admitted scalar numeric cells
- admitted scalar text cells
- admitted ordinary formula cells

The path intentionally rejects:

- shared-group formulas
- matrix formulas
- scalar payload classes outside the admitted slice

That keeps mirroring aligned with the contract rather than turning it into a
silent widening mechanism.

## Validation Surface

The landed Calc proof lanes are:

- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  - `testComputationalCellStorageMirrorRebuildsLifecycleState`
  - `testComputationalCellStorageMirrorRebuildsStructuralState`

Those tests now prove the bounded host path:

- damage the admitted live Calc cell surface
- mirror the admitted engine-resident cells back into Calc
- rebuild the admitted live wiring surface from engine-owned graph targets
- verify exact computational and graph agreement with the engine-owned
  after-state

## Boundary Kept Intact

This workstream still does not claim:

- formula-cell object lifetime migration
- live broadcaster/listener container residency migration
- broad `ScDocument` storage residency migration
- widening beyond the admitted scalar lifecycle and admitted row/column
  structural slice

The mirroring path is a host realization step for engine-owned admitted cell
state, not a broad storage transplant.
