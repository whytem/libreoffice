# Computational Substrate Scalar Mutation Entry Evidence

Status: frozen scalar-entry convergence evidence

## Purpose

This note records the bounded proof cycle for
[COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md)
after the scalar authority after-state was updated to carry the dependency-
derived broadcaster surface.

## Before And After

The observation workstream froze the pre-fix scalar mismatch as:

- broadcaster classification: `missing_expected_broadcasters`
- expected cell broadcasters: `0`
- live cell broadcasters: `2`
- exact queue comparison: yes
- exact graph comparison: yes
- exact computational comparison: no

After the convergence change:

- broadcaster classification: `exact`
- exact queue comparison: yes
- exact graph comparison: yes
- exact computational comparison: yes

The scalar admitted mutation-entry lane now closes with full computational
agreement instead of a broadcaster-only caveat.

## Exact Proof Surface

The bounded scalar proof surface stayed the same:

- admitted direct `SetScalarValue`
- clean baseline
- ordinary scalar formulas only
- no shared groups
- no named-range-sensitive widening

On that surface, the scalar mutation-entry lane now proves:

- exact queue verification
- exact computational verification
- exact graph verification
- exact broadcaster canonicalization

The existing formula-entry and admitted structural-entry proof lanes remain
green as before.

## Retained Safety Coverage

The retained safety lanes stayed green during this cycle:

- dirty-baseline rejection for direct mutation entry
- repair-detected classification and rollback coverage
- full dependency-shadow regression coverage
- standalone computational substrate coverage

This matters because the scalar convergence fix did not weaken rollback or
broaden the admitted surface to get to green.

## Operational Sample

One bounded scalar-entry sample was captured from:

- `make -j1 CppunitTest_sc_ucalc_dependency_shadow CPPUNIT_TEST_NAME=testComputationalMutationEntrySetValue`

Observed sample:

- elapsed: `7.70s`
- max RSS: `248684 KB`

This is only a bounded operational sample for the scalar proof lane, not a
global performance claim.

## Validation Used

The evidence freeze stayed green on:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Standing replay baseline remains exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

## Evidence Conclusion

The scalar-entry broadcaster gap is closed on the admitted slice.

The evidence now supports reassessing the earlier validation-only mutation-
entry decision, because the remaining blocker recorded in the prior closeout
is no longer present on the bounded scalar proof surface.
