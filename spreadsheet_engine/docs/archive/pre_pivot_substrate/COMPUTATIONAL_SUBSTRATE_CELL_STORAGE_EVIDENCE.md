# Computational Substrate Cell Storage Evidence

Status: frozen differential evidence note

## Exact Engine-Resident Cell Proof

The admitted cell-storage residency path is now proven on the bounded slice by
the Calc-side mirror tests in
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx):

- `testComputationalCellStorageMirrorRebuildsLifecycleState`
- `testComputationalCellStorageMirrorRebuildsStructuralState`

Those tests:

- build the engine-owned before shadow, graph shadow, and transition
- advance the engine-owned mutable substrate into its resident cell after-state
- damage the admitted live Calc cell surface
- mirror the admitted engine-resident cells back into Calc
- rebuild the admitted live wiring surface from engine-owned graph targets
- verify exact computational and graph agreement against the engine-owned
  after-state

The resulting exact verdict is:

- lifecycle cell mirroring: exact
- structural cell mirroring: exact

## Exact Resident-Store State Proof

The engine-side resident-store proof lane now lives in
[computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx).

That lane now verifies:

- bootstrap exactness between the resident store and the admitted initial
  computational shadow
- exact resident-store updates after admitted scalar authority transitions
- exact resident-store updates after admitted lifecycle insert and remove
  transitions
- exact resident-store updates after admitted structural transitions

This is the point where admitted cell residency stops being implicit mutable
facade state and becomes an explicit engine-owned storage surface.

## Retained Safety Evidence

The cell-storage path still inherits the standing bounded safety model from
the earlier authority and storage-and-wiring proof cycles.

The standing green Calc proof lanes still include:

- `testComputationalLifecycleClassifiesRepairDetected`
- `testComputationalStructuralRepairDetectedRollback`
- `testComputationalStructuralDeleteRowRepairDetectedRollback`
- `testComputationalStructuralInsertColumnRepairDetectedRollback`

That means the cell-storage proof still has checked-in evidence for:

- explicit repair detection
- explicit rollback on structural divergence
- no silent widening beyond the admitted slice

## Operational Sample

One focused lifecycle sample from the admitted cell-storage mirror lane:

- `CppunitTest_sc_ucalc_dependency_shadow`
  with `CPPUNIT_TEST_NAME=testComputationalCellStorageMirrorRebuildsLifecycleState`
  measured `elapsed=7.94` and `rss_kb=248752`

One focused structural sample:

- `CppunitTest_sc_ucalc_dependency_shadow`
  with `CPPUNIT_TEST_NAME=testComputationalCellStorageMirrorRebuildsStructuralState`
  measured `elapsed=8.01` and `rss_kb=248728`

These numbers are evidence snapshots, not broad performance claims. They are
useful as bounded operational references for the first engine-resident
cell-storage path.

## Assessment

The admitted cell-storage residency path is cleaner than the prior
mutable-sidecar-only boundary in one important sense:

- the engine now owns the resident admitted cell state
- the engine still owns graph and wiring target decisions
- Calc is reduced to mirrored cell realization, retained live container
  realization, and final verification

The path is still intentionally narrow and not yet a justification for broad
`ScDocument` storage migration. But it is now a real cell-residency boundary,
not just a mutable sidecar paired with host-owned physical cell storage.
