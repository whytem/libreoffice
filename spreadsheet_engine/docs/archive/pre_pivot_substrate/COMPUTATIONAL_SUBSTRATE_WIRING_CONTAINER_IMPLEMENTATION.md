# Computational Substrate Wiring Container Implementation

Status: completed implementation note

## Purpose

This note records the first engine-owned admitted-slice wiring-container
implementation for
[COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md).

The implementation goal for this workstream is narrower than live container
migration in full Calc:

- introduce an explicit engine-owned resident wiring-container store
- bootstrap it from the engine-owned dependency graph shadow
- keep it synchronized from engine-authored graph deltas for admitted
  authority, lifecycle, and structural transitions
- prove that it stays exact on the admitted slice before any Calc realization
  path consumes it directly

## Landed Engine-Owned Wiring Shape

The resident wiring store now lives in
[MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx).

The landed surface is:

- `AdmittedWiringContainers`
- `AdmittedWiringContainerComparison`
- `buildAdmittedWiringContainers(...)`
- `compareAdmittedWiringContainers(...)`

The mutable substrate state now carries:

- `maGraphShadow`
- `maWiringContainers`

alongside the already-proven:

- `maFacade`
- `maObservation`
- `maShadow`
- `maCellStorage`

This makes admitted live wiring residency explicit rather than treating graph
after-target vectors as the only durable representation.

## Synchronization Model

The resident wiring store now advances from engine-owned graph transitions,
not from rereading Calc after each mutation.

The landed synchronization model is:

- bootstrap from the initial dependency graph shadow
- reconcile after admitted authority transitions from graph deltas
- reconcile after admitted lifecycle transitions from graph deltas
- reconcile after admitted structural transitions from graph deltas

The reconciliation path is intentionally value-semantic:

- broadcaster nodes are updated by normalized broadcaster id
- listener edges are updated by normalized edge identity
- formula-tree and formula-track realized order are refreshed from the
  engine-owned after-state
- resident workbook generation is updated from the engine-owned graph snapshot

This is still a bounded admitted-slice implementation, but it is no longer
just a replay bundle.

## Validation Surface

The landed proof lanes are:

- standalone resident wiring-store checks in
  [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- standalone graph-delta checks in
  [computational_graph_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_graph_tests.cxx)

The new checks verify:

- bootstrap exactness between resident wiring containers and the admitted
  graph shadow
- exact resident-store updates after admitted scalar authority transitions
- exact resident-store updates after admitted lifecycle insert and remove
  transitions
- exact resident-store updates after admitted structural transitions on the
  existing bounded slice

## Boundary Kept Intact

This implementation still does not claim:

- Calc realization directly from resident wiring containers
- formula-cell object lifetime migration
- broader dependency-container migration outside the admitted slice
- shared-group or named-range-sensitive live wiring residency

Those remain later workstreams in the same plan.
