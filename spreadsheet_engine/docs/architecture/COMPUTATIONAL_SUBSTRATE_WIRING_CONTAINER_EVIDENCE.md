# Computational Substrate Wiring Container Evidence

Status: frozen differential evidence note

## Exact Engine-Resident Wiring Proof

The admitted wiring-container residency path is now proven on the bounded
slice by the Calc-side resident realization tests in
[ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx):

- `testComputationalCellStorageMirrorRebuildsLifecycleState`
- `testComputationalCellStorageMirrorRebuildsStructuralState`

Those tests now prove the combined admitted resident path:

- build the engine-owned before shadow, graph shadow, and transition
- advance the engine-owned mutable substrate into its resident cell and
  resident wiring after-state
- damage the admitted live Calc cell and wiring surface
- mirror admitted live cells from `maCellStorage`
- realize admitted live wiring from `maWiringContainers`
- verify exact computational and graph agreement against the engine-owned
  after-state

The resulting exact verdict is:

- lifecycle resident wiring realization: exact
- structural resident wiring realization: exact

## Exact Resident-Store State Proof

The engine-side resident wiring proof lanes now live in:

- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- [computational_graph_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_graph_tests.cxx)

Those lanes now verify:

- bootstrap exactness between resident wiring containers and the admitted
  graph shadow
- exact resident-store updates after admitted scalar authority transitions
- exact resident-store updates after admitted lifecycle insert and remove
  transitions
- exact resident-store updates after admitted structural transitions
- replay consistency between graph deltas and resident wiring-store advance

This is the point where admitted live wiring stops being just a replay target
bundle and becomes explicit engine-owned resident state.

## Retained Safety Evidence

The wiring-container residency path still inherits the standing bounded safety
model from the earlier authority, storage-and-wiring, and cell-residency
proof cycles.

The standing green Calc proof lanes still include:

- `testComputationalLifecycleClassifiesRepairDetected`
- `testComputationalStructuralRepairDetectedRollback`
- `testComputationalStructuralDeleteRowRepairDetectedRollback`
- `testComputationalStructuralInsertColumnRepairDetectedRollback`

That means the resident wiring proof still has checked-in evidence for:

- explicit repair detection
- explicit rollback on structural divergence
- no silent widening beyond the admitted slice

## Operational Sample

One focused lifecycle sample from the admitted resident cell-plus-wiring lane:

- `CppunitTest_sc_ucalc_dependency_shadow`
  with `CPPUNIT_TEST_NAME=testComputationalCellStorageMirrorRebuildsLifecycleState`
  measured `elapsed=6.24` and `rss_kb=248728`

One focused structural sample:

- `CppunitTest_sc_ucalc_dependency_shadow`
  with `CPPUNIT_TEST_NAME=testComputationalCellStorageMirrorRebuildsStructuralState`
  measured `elapsed=6.23` and `rss_kb=248632`

These numbers are evidence snapshots, not broad performance claims. They are
useful as bounded operational references for the first engine-resident
wiring-container realization path.

## Assessment

The admitted wiring-container residency path is materially cleaner than the
older graph-target-only boundary in one important sense:

- the engine now owns resident admitted cell storage
- the engine now owns resident admitted wiring containers
- the engine still owns graph, queue, and after-state decisions
- Calc is reduced to live realization, formula-cell object lifetime, and
  final verification

The path is still intentionally narrow and not yet a justification for broad
listener/broadcaster residency migration across all workbook classes. But it
is now a real admitted live-wiring residency boundary, not just a replay
bundle paired with host-owned live containers.
