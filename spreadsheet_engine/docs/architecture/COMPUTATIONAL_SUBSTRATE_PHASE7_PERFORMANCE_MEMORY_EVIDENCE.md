# Computational Substrate Phase 7 Performance And Memory Evidence

Status: active performance and memory evidence note for Phase 7

## Purpose

This note records the coarse performance and memory observations gathered for
the Phase 7 rollout decision.

These measurements are environment-specific and should be treated as decision
evidence, not as product benchmarks. Their job is to show whether the bounded
rollout candidate appears operationally sane enough to justify narrow proceed.

## Measured Commands

The following commands were measured on the Phase 7 branch state:

- `/usr/bin/time -f 'elapsed=%e maxrss_kb=%M exit=%x' make -j1 CPPUNIT_TEST_NAME=testComputationalLifecycleInsertFormulaPilot CppunitTest_sc_ucalc_dependency_shadow`
- `/usr/bin/time -f 'elapsed=%e maxrss_kb=%M exit=%x' make -j1 CPPUNIT_TEST_NAME=testComputationalStructuralInsertRowPilot CppunitTest_sc_ucalc_dependency_shadow`
- `/usr/bin/time -f 'elapsed=%e maxrss_kb=%M exit=%x' make -j1 CPPUNIT_TEST_NAME=testComputationalStructuralRepairDetectedRollback CppunitTest_sc_ucalc_dependency_shadow`
- `/usr/bin/time -f 'elapsed=%e maxrss_kb=%M exit=%x' spreadsheet_engine/build_check/spreadsheetengine_computational_substrate_tests`
- `/usr/bin/time -f 'elapsed=%e maxrss_kb=%M exit=%x' spreadsheet_engine/build_check/spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

## Observed Results

- `testComputationalLifecycleInsertFormulaPilot`
  - `elapsed=8.17`
  - `maxrss_kb=248712`
  - `exit=0`
- `testComputationalStructuralInsertRowPilot`
  - `elapsed=8.23`
  - `maxrss_kb=248696`
  - `exit=0`
- `testComputationalStructuralRepairDetectedRollback`
  - `elapsed=8.12`
  - `maxrss_kb=248760`
  - `exit=0`
- `spreadsheetengine_computational_substrate_tests`
  - `elapsed=0.00`
  - `maxrss_kb=4508`
  - `exit=0`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
  - `elapsed=59.61`
  - `maxrss_kb=26160`
  - `exit=0`

## Interpretation

These measurements support a narrow proceed more than they support either a
broad proceed or a stop:

- the admitted substrate executable lane is very light
- the targeted Calc-hosted pilot runs do not show an obvious memory blow-up
  beyond the normal Cppunit host process footprint
- the standing promoted replay corpus remains zero-fallback while staying
  within a manageable memory envelope for the measured run

What these measurements do not prove:

- a broad production-ready performance case for all workbook classes
- memory behavior for shared-group, named-range-sensitive, or load-time heavy
  flows
- anything about multi-document or user-interactive latency

## Decision Use

The measured evidence is good enough to reject an immediate stop on
performance or memory grounds for the narrow rollout candidate.

It is not strong enough to justify broad rollout beyond the admitted Phase 6
surface.
