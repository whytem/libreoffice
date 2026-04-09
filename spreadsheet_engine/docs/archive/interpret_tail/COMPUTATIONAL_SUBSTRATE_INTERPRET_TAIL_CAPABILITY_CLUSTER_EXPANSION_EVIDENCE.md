# Computational Substrate InterpretTail Capability-Cluster Expansion Evidence

Status: completed evidence summary for the second live evaluator switchover wave

## Evidence Summary

This pass closed with both runtime change and live Calc proof.

The evidence buckets now cover:

- bounded host-backed scalar materialization on the live `InterpretTail`
  seam
- workbook-local single-cell references
- workbook-global and sheet-local single-cell names
- simple scalar expression trees used as text-parsing inputs
- bounded lookup/index routing with authoritative string projection
- explicit fallback accounting for unsupported functions and unsupported host
  surfaces

## Targeted Proof

The key targeted proof buckets are:

- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)
  proves the helper path for:
  - `DATEVALUE(A1)`
  - `DATEVALUE("1954-"&A2)`
  - `TIMEVALUE(MyTimeName)`
  - `VALUE(-A3)`
  - `NUMBERVALUE(A8;B8;C8)`
  - bounded `MATCH`
  - bounded `VLOOKUP`
  - bounded `INDEX`
- [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)
  proves the live AutoCalc seam for:
  - `observe` classification on widened text parsing
  - `shadow` comparison on bounded lookup
  - `authority` routing for global-name and sheet-local-name `TIMEVALUE`
  - `authority` routing for bounded `VLOOKUP`
  - explicit fallback on unsupported functions

## Validation

The following validation passed for this wave:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

The standing replay baseline remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

## Practical Outcome

The pass widened real engine-first authority inside
[formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
instead of merely adding oracle coverage. That is the defining evidence for
accepting this as a migration wave rather than a verification-only pass.

