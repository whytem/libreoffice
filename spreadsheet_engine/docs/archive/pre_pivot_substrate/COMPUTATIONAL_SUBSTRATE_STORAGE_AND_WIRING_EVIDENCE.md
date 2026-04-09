# Computational Substrate Storage And Wiring Evidence

Status: completed differential evidence note

## Exact Engine-Driven Apply Proof

The admitted live wiring apply path is now proven on the bounded slice by the
Calc-side replay tests in
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx):

- `testComputationalWiringApplyRebuildsLifecycleState`
- `testComputationalWiringApplyRebuildsStructuralState`

Those tests:

- build the engine-owned before shadow, graph shadow, and IR shadow
- construct the engine-owned lifecycle or structural transition
- derive the graph-wiring delta bundle
- clear the retained Calc listener/tree/track surface
- rebuild that live host surface from the engine-owned target sets
- verify the rebuilt live state against the engine-owned after-graph with an
  exact graph comparison

The resulting exact verdict is:

- lifecycle apply: exact
- structural apply: exact

## Retained Safety Evidence

The storage-and-wiring path still inherits the existing bounded safety model.
The standing Calc proof lanes that remain green include:

- `testComputationalLifecycleClassifiesRepairDetected`
- `testComputationalStructuralRepairDetectedRollback`
- `testComputationalStructuralDeleteRowRepairDetectedRollback`
- `testComputationalStructuralInsertColumnRepairDetectedRollback`

That means the project still has checked-in evidence for:

- explicit repair detection
- explicit rollback on structural divergence
- no silent promotion beyond the admitted slice

## Standalone Evidence

The standalone regression lanes that exercise the new engine-owned sidecar and
graph-delta surfaces are:

- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_computational_graph_tests`

These lanes now validate:

- mutable sidecar advancement from engine-owned authority, lifecycle, and
  structural transitions
- graph delta generation from engine-owned before/after graph shadows
- replay consistency between transition-built and mutable-state-built deltas

## Operational Sample

One focused operational sample from the lifecycle wiring-rebuild lane:

- `CppunitTest_sc_ucalc_dependency_shadow`
  with `CPPUNIT_TEST_NAME=testComputationalWiringApplyRebuildsLifecycleState`
  measured `elapsed=7.74` and `rss_kb=248636`

One focused structural sample:

- `CppunitTest_sc_ucalc_dependency_shadow`
  with `CPPUNIT_TEST_NAME=testComputationalWiringApplyRebuildsStructuralState`
  measured `elapsed=7.60` and `rss_kb=248760`

These numbers are evidence snapshots, not broad performance claims. They are
useful as a bounded operational reference for the first engine-driven host
rebuild path.

## Assessment

The engine-driven storage-and-wiring path is cleaner than the earlier
rebuild-and-verify model in one important sense:

- the engine now authors the mutable sidecar state
- the engine now authors the graph and wiring target sets
- Calc is reduced to container realization and final verification

The path is still intentionally narrow and not yet a justification for broad
cell-storage or listener-container migration. But it is now a real authority
surface, not just a read-only shadowing exercise.
