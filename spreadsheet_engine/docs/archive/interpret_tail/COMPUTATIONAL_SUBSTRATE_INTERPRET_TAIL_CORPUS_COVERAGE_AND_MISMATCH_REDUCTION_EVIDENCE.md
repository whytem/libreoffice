# Computational Substrate InterpretTail Corpus Coverage And Mismatch Reduction Evidence

Status: completed evidence summary for the focused InterpretTail corpus
coverage and mismatch-reduction pass

## Evidence Summary

This pass closed with:

- runtime widening on the current promoted evaluator families
- stronger per-function and per-reason corpus accounting
- a rerun that materially improved authoritative usage and
  unsupported-shape fallout
- a flatter but more trustworthy view of the real probe-covered promoted
  family surface on the current replay corpus

## Frozen Baseline Versus Completed Snapshot

Frozen starting point:

- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=996`
- `interpret_tail_authoritative_fallback_total=395`
- `interpret_tail_fallback_unsupported_formula_shape=295`
- `interpret_tail_fallback_shadow_mismatch=85`
- `interpret_tail_fallback_unsupported_host_surface=15`

Completed rerun:

- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=1045`
- `interpret_tail_authoritative_fallback_total=346`
- `interpret_tail_fallback_unsupported_formula_shape=240`
- `interpret_tail_fallback_shadow_mismatch=90`
- `interpret_tail_fallback_unsupported_host_surface=16`

Net change:

- authoritative routes: `+49`
- total fallbacks: `-49`
- unsupported-shape fallbacks: `-55`
- probe-covered promoted-family cells: unchanged
- shadow mismatches: `+5`
- unsupported host surface: `+1`

## Per-Family Completed Snapshot

Per promoted family, the completed corpus snapshot is:

- `VALUE`: authoritative `14`, fallback `1`, attempts `15`
- `DATEVALUE`: authoritative `4`, fallback `27`, attempts `31`
- `TIMEVALUE`: authoritative `8`, fallback `0`, attempts `8`
- `NUMBERVALUE`: authoritative `9`, fallback `0`, attempts `9`
- `MATCH`: authoritative `101`, fallback `16`, attempts `117`
- `XMATCH`: authoritative `24`, fallback `9`, attempts `33`
- `LOOKUP`: authoritative `550`, fallback `265`, attempts `815`
- `VLOOKUP`: authoritative `294`, fallback `19`, attempts `313`
- `HLOOKUP`: authoritative `39`, fallback `0`, attempts `39`
- `INDEX`: authoritative `2`, fallback `9`, attempts `11`

The largest retained per-family contributors are now explicit:

- `DATEVALUE`: `27` shadow mismatches
- `LOOKUP`: `217` unsupported-shape fallbacks and `45` shadow mismatches
- `VLOOKUP`: `8` unsupported-host-surface fallbacks and `9` shadow
  mismatches
- `INDEX`: `4` unsupported-shape fallbacks and `5`
  unsupported-host-surface fallbacks

## Runtime And Proof Surfaces

The key runtime work landed in:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)

The pass widened the current seam by adding:

- bounded matrix and array-constant materialization for lookup-family input
  sources
- bounded scalar-node materialization as `1x1` matrix sources
- bounded `INDEX` matrix-selection support on the current scalar result
  surface
- bounded `1x1` matrix result projection
- Calc-consistent `VALUE(empty-cell)` handling
- per-function and per-fallback-reason corpus accounting

The targeted proof surfaces are:

- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)
- [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)
- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)

Those buckets now prove:

- bounded live helper coverage for the new matrix, array-constant, and
  `VALUE(empty-cell)` behaviors
- live authoritative-with-fallback carry-through on new bounded examples
- Calc-backed replay-corpus stats with per-function and per-reason totals

## What The Flat Probe Count Means

This pass intentionally tried to raise
`interpret_tail_probe_formula_cells` by aligning probe classification with
the same live formula-source normalization used by the delegated seam.

The rerun kept the count at `1391`.

That is accepted as evidence, not failure to instrument.

It means the current corpus is already exposing the same top-level promoted
family roots that the live seam can currently recognize on this bounded
surface. Additional improvement now needs to come primarily from converting
existing attempts into authoritative routes, not from hoping a hidden block
of probe-eligible cells will appear.

## Validation

The following validation passed for this wave:

- `CPPUNIT_TEST_NAME=testInterpretTailEngineEvaluatorHelper CppunitTest_sc_ucalc_shared_cases`
- `CPPUNIT_TEST_NAME=testInterpretTailEngineEvaluatorAuthoritativeWithFallback CppunitTest_sc_ucalc_formula2`
- `SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_STATS=1 CPPUNIT_TEST_NAME=testAuthorityStats CppunitTest_sc_interpret_tail_corpus`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

The standing standalone replay baseline remained exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`
