# Computational Substrate InterpretTail Authoritative Usage Expansion Evidence

Status: completed evidence summary for the ambitious InterpretTail
authoritative-usage expansion pass

## Evidence Summary

This pass closed with:

- real runtime widening on the live `InterpretTail -> engine` seam
- bounded live `XLOOKUP` authority
- bounded wrapper-aware live delegation for `IFERROR` / `IFNA`
- a larger Calc-backed supported-family probe surface
- a higher authoritative corpus total
- a mixed hotspot outcome rather than a full cleanup win

## Frozen Baseline Versus Completed Snapshot

Frozen starting point:

- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=1045`
- `interpret_tail_authoritative_fallback_total=346`
- `interpret_tail_fallback_unsupported_formula_shape=240`
- `interpret_tail_fallback_shadow_mismatch=90`
- `interpret_tail_fallback_unsupported_host_surface=16`

Completed rerun:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1126`
- `interpret_tail_authoritative_fallback_total=362`
- `interpret_tail_fallback_unsupported_formula_shape=243`
- `interpret_tail_fallback_shadow_mismatch=97`
- `interpret_tail_fallback_unsupported_host_surface=22`

Net change:

- probe-covered promoted-family cells: `+97`
- authoritative routes: `+81`
- total fallbacks: `+16`
- unsupported-shape fallbacks: `+3`
- shadow mismatches: `+7`
- unsupported host surface: `+6`

## Per-Family Completed Snapshot

Per promoted family, the completed corpus snapshot is:

- `VALUE`: authoritative `14`, fallback `1`, attempts `15`
- `DATEVALUE`: authoritative `4`, fallback `27`, attempts `31`
- `TIMEVALUE`: authoritative `8`, fallback `0`, attempts `8`
- `NUMBERVALUE`: authoritative `9`, fallback `0`, attempts `9`
- `MATCH`: authoritative `102`, fallback `15`, attempts `117`
- `XMATCH`: authoritative `24`, fallback `9`, attempts `33`
- `LOOKUP`: authoritative `550`, fallback `265`, attempts `815`
- `VLOOKUP`: authoritative `296`, fallback `17`, attempts `313`
- `HLOOKUP`: authoritative `39`, fallback `0`, attempts `39`
- `XLOOKUP`: authoritative `78`, fallback `19`, attempts `97`
- `INDEX`: authoritative `2`, fallback `9`, attempts `11`

The main retained per-family contributors are now:

- `DATEVALUE`: `27` shadow mismatches
- `LOOKUP`: `217` unsupported-shape fallbacks and `45` shadow mismatches
- `VLOOKUP`: `8` unsupported-host-surface fallbacks and `7` shadow
  mismatches
- `XLOOKUP`: `6` unsupported-host-surface fallbacks and `10` shadow
  mismatches
- `INDEX`: `4` unsupported-shape fallbacks and `5`
  unsupported-host-surface fallbacks

## Runtime And Proof Surfaces

The key runtime work landed in:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)

The completed pass widened the current seam by adding:

- bounded live `XLOOKUP` request construction and execution
- bounded delegated scalar fallback for `XLOOKUP(...; if_not_found)`
- wrapper-aware delegated evaluation for `IFERROR` / `IFNA`
- wider probe recognition through the same delegated-root classifier
- additional bounded `DATEVALUE` helper support for referenced date-shaped
  inputs and ISO datetime text

The targeted proof surfaces are:

- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)
- [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)
- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)

Those buckets now prove:

- bounded helper support for live `XLOOKUP`
- bounded helper support for wrapper-aware delegated roots
- live authoritative-with-fallback carry-through on the widened seam
- Calc-backed replay-corpus stats that now include non-zero `XLOOKUP`
  authoritative usage

## Interpretation

The important positive result is that the live seam is no longer limited to
the previous top-level lookup cluster.

The corpus now records:

- a larger probe-covered live evaluator surface
- a higher authoritative total
- a real newly-authoritative family: `XLOOKUP`

The important negative result is that the pass did not convert the dominant
retained hotspots as aggressively as planned.

So this evidence supports a mixed-result closeout:

- real widening win
- incomplete hotspot-conversion win

## Validation

The following validation passed for this wave:

- `CPPUNIT_TEST_NAME=testInterpretTailEngineEvaluatorHelper CppunitTest_sc_ucalc_shared_cases`
- `CPPUNIT_TEST_NAME=testInterpretTailEngineEvaluatorSourceNormalization CppunitTest_sc_ucalc_shared_cases`
- `CPPUNIT_TEST_NAME=testInterpretTailEngineEvaluatorAuthoritativeWithFallback CppunitTest_sc_ucalc_formula2`
- `CPPUNIT_TEST_NAME=testInterpretTailEngineEvaluatorObserveAndShadowCompare CppunitTest_sc_ucalc_formula2`
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
