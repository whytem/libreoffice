# Computational Substrate Wiring Container Realization

Status: completed implementation note

## Purpose

This note records the first Calc realization path for engine-resident
wiring containers introduced by
[COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md).

The realization goal for this workstream is narrower than full host removal:

- materialize admitted live listener, formula-tree, and formula-track state
  from engine-resident wiring containers
- keep resident admitted cell storage as the separate engine-owned source of
  truth for live cells
- keep Calc as the temporary mutation-entry, formula-cell-lifetime, and
  rollback host
- ensure realized live wiring follows resident engine state rather than
  reconstructing directly from graph deltas

## Landed Calc Realization Surface

The landed host adapter lives in
[ComputationalSubstrateWiring.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx).

The realization surface is now:

- `realizeAdmittedWiringContainers(...)`

The compat layer still retains:

- `rebuildAdmittedLiveWiring(...)`

but it is now a compatibility wrapper. The resident-store path is the primary
realization entry for this plan.

## Realization Model

The landed realization model is:

- collect admitted live formula cells from Calc
- validate the resident wiring store against admitted formula anchors and
  listener-count consistency
- clear retained live wiring on the admitted slice
- replay listener edges from the resident store
- replay formula-tree realized order from the resident store
- replay formula-track realized order from the resident store

This keeps the after-state decision engine-owned:

- Calc realizes engine-resident wiring state
- Calc does not recompute that wiring state locally

## Validation Surface

The landed Calc proof lane is in
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx).

The admitted cell-plus-wiring mirror tests now validate:

- resident admitted cell mirroring from `maCellStorage`
- resident admitted wiring realization from `maWiringContainers`
- exact computational comparison after realization
- exact graph comparison after realization
- exact broadcaster-node count handoff from the resident store

The key proof shape is now:

- engine-owned mutable state advances
- Calc live cells are mirrored from engine-resident cell storage
- Calc live wiring is realized from engine-resident wiring containers
- exact bounded verification still passes

## Boundary Kept Intact

This realization path still does not claim:

- formula-cell object lifetime migration
- direct engine-owned `ScDocument` mutation entry
- broad listener/broadcaster container replacement outside the admitted slice
- named-range-sensitive, shared-group-sensitive, or sheet-wide live wiring
  residency

Those remain later decisions after the admitted resident realization path is
fully evidenced.
