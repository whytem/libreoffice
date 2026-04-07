# Computational Substrate Shared-Group Named-Range Live Ownership Evidence

Status: frozen evidence for bounded named-range-combined live ownership

## What The Evidence Shows

- bounded named-range target expressions no longer create opaque dependency
  nodes or edges
- bounded shared-group named-range `SameTextPreserve` now applies on the
  live lifecycle path
- the same bounded family now applies on the live mutation-entry path
- bounded named-range-combined member-exit still stays out of contract

## Proof Buckets

- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)
  proves zero-opacity named-range snapshot, live lifecycle apply, live
  mutation-entry apply, and retained member-exit reject
- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
  already contained the bounded standalone exact preserve proof

## Verification Commands

- `make -j1 CPPUNIT_TEST_NAME=testNamedRangeInvalidationShadow CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalNarrowRolloutSharedGroupNonStructuralLifecycleNamedRangeSameTextPreserveApplies CppunitTest_sc_ucalc_dependency_shadow`
- `make -j1 CPPUNIT_TEST_NAME=testComputationalMutationEntrySharedGroupNonStructuralNamedRangeSameTextPreserveApplies CppunitTest_sc_ucalc_dependency_shadow`
