# Computational Substrate Formula-Cell Lifetime Evidence

Status: frozen differential evidence note

## Exact Engine-Owned Lifetime Proof

The admitted formula-cell lifetime path is now proven on the bounded slice by
the Calc-side realization tests in
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx):

- `testComputationalFormulaCellLifetimeRealizesLifecycleInsertState`
- `testComputationalFormulaCellLifetimeRealizesLifecycleRemoveState`
- `testComputationalFormulaCellLifetimeRealizesStructuralState`

Those tests now prove the combined admitted resident path:

- build the engine-owned before shadow, graph shadow, and transition
- advance the mutable substrate into its admitted lifetime, resident cell, and
  resident wiring after-state
- damage or remove the live Calc formula-cell objects
- realize formula-cell object lifetime from `maFormulaCellLifetime`
- replay only scalar cell values from `maCellStorage`
- realize live wiring from `maWiringContainers`
- verify exact computational, graph, and queue agreement against the
  engine-owned after-state

The resulting exact verdict is:

- lifecycle insert realization: exact
- lifecycle remove realization: exact
- structural realization: exact

## Exact Resident-State Proof

The engine-side resident lifetime proof lane remains in
[computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx).

That lane verifies:

- bootstrap exactness between resident formula-cell lifetime and the admitted
  initial computational shadow
- exact lifetime-store updates after admitted authority transitions
- exact lifetime-store updates after admitted lifecycle insert and remove
  transitions
- exact lifetime-store updates after admitted structural transitions

This is the point where admitted formula-cell object lifetime stops being a
host-only consequence of cell mirroring and becomes a separately owned
engine-resident surface.

## Queue And Differential Behavior

The Calc-side realization tests now explicitly prove that the queue predicted
by the engine-authored transition still matches Calc’s realized formula-tree
order after formula-cell lifetime realization.

That matters because this workstream changes the live formula-object boundary
without loosening the earlier admitted-slice verification rules:

- exact computational agreement still holds
- exact graph agreement still holds
- exact queue agreement still holds
- live rollback remains available through the existing host path

## Retained Safety Evidence

The formula-cell lifetime path still inherits the standing bounded safety
model from the earlier authority, structural, resident-cell, and
resident-wiring proof cycles.

The standing green Calc proof lanes still include:

- `testComputationalLifecycleClassifiesRepairDetected`
- `testComputationalStructuralRepairDetectedRollback`
- `testComputationalStructuralDeleteRowRepairDetectedRollback`
- `testComputationalStructuralInsertColumnRepairDetectedRollback`

That means the formula-cell lifetime proof still has checked-in evidence for:

- explicit repair detection
- explicit rollback on divergence
- no silent widening beyond the admitted slice

## Operational Sample

Representative lifecycle samples:

- `CppunitTest_sc_ucalc_dependency_shadow`
  with `CPPUNIT_TEST_NAME=testComputationalCellStorageMirrorRebuildsLifecycleState`
  measured `elapsed=7.52` and `rss_kb=248732`
- `CppunitTest_sc_ucalc_dependency_shadow`
  with `CPPUNIT_TEST_NAME=testComputationalFormulaCellLifetimeRealizesLifecycleInsertState`
  measured `elapsed=7.51` and `rss_kb=248632`

Representative structural samples:

- `CppunitTest_sc_ucalc_dependency_shadow`
  with `CPPUNIT_TEST_NAME=testComputationalCellStorageMirrorRebuildsStructuralState`
  measured `elapsed=7.44` and `rss_kb=248636`
- `CppunitTest_sc_ucalc_dependency_shadow`
  with `CPPUNIT_TEST_NAME=testComputationalFormulaCellLifetimeRealizesStructuralState`
  measured `elapsed=5.12` and `rss_kb=248768`

These numbers are bounded operational samples, not broad performance claims.
They are useful as a focused reference for the first engine-owned
formula-cell lifetime realization path.

## Assessment

The admitted formula-cell lifetime path is materially cleaner than the prior
resident-state-plus-host-lifetime boundary in one important sense:

- the engine now owns admitted resident cell storage
- the engine now owns admitted resident wiring containers
- the engine now owns admitted formula-cell lifetime state
- Calc is reduced to realization, mutation entry, and final rollback on this
  slice

The path is still intentionally narrow and is not yet a justification for
broad formula-cell object migration outside the admitted slice. But it is now
a real engine-owned lifetime boundary rather than resident storage paired with
implicitly host-owned formula objects.
