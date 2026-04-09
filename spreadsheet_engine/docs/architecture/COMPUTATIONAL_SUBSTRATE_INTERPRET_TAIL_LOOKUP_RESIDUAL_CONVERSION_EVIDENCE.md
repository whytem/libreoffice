# Computational Substrate InterpretTail Lookup Residual Conversion Evidence

Status: completed evidence summary for the focused InterpretTail
lookup-residual conversion pass

## Evidence Summary

This pass closed with:

- real runtime widening on the already-admitted live `InterpretTail ->
  engine` seam
- a large authoritative-usage increase on the same measured corpus surface
- near-elimination of the old `LOOKUP` unsupported-shape hotspot
- a smaller but still real reduction in residual mismatch
- retained lookup-family host-surface fallout that is now much narrower than
  the starting boundary

## Frozen Baseline Versus Completed Snapshot

Frozen starting point:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1156`
- `interpret_tail_authoritative_fallback_total=332`
- `interpret_tail_fallback_unsupported_formula_shape=241`
- `interpret_tail_fallback_shadow_mismatch=72`
- `interpret_tail_fallback_unsupported_host_surface=19`

Completed rerun:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1379`
- `interpret_tail_authoritative_fallback_total=109`
- `interpret_tail_fallback_unsupported_formula_shape=26`
- `interpret_tail_fallback_shadow_mismatch=64`
- `interpret_tail_fallback_unsupported_host_surface=19`

Net change:

- probe-covered promoted-family cells: `+0`
- authoritative routes: `+223`
- total fallbacks: `-223`
- unsupported-shape fallbacks: `-215`
- shadow mismatches: `-8`
- unsupported host surface: `+0`

## Per-Family Completed Snapshot

Per promoted family, the completed corpus snapshot is:

- `VALUE`: authoritative `14`, fallback `1`, attempts `15`
- `DATEVALUE`: authoritative `29`, fallback `2`, attempts `31`
- `TIMEVALUE`: authoritative `8`, fallback `0`, attempts `8`
- `NUMBERVALUE`: authoritative `9`, fallback `0`, attempts `9`
- `MATCH`: authoritative `102`, fallback `15`, attempts `117`
- `XMATCH`: authoritative `24`, fallback `9`, attempts `33`
- `LOOKUP`: authoritative `777`, fallback `38`, attempts `815`
- `VLOOKUP`: authoritative `296`, fallback `17`, attempts `313`
- `HLOOKUP`: authoritative `39`, fallback `0`, attempts `39`
- `XLOOKUP`: authoritative `78`, fallback `19`, attempts `97`
- `INDEX`: authoritative `2`, fallback `9`, attempts `11`

The main retained per-family contributors are now:

- `LOOKUP`: `38` shadow mismatches
- `VLOOKUP`: `8` unsupported-host-surface fallbacks and `7` shadow
  mismatches
- `XLOOKUP`: `6` unsupported-host-surface fallbacks and `10` shadow
  mismatches
- `INDEX`: `4` unsupported-shape fallbacks and `5`
  unsupported-host-surface fallbacks
- `MATCH`: `9` unsupported-shape fallbacks and `6` shadow mismatches

The most important per-family hotspot change is `LOOKUP` unsupported-shape:

- before: authoritative `555`, fallback `260`, unsupported-shape `215`
- after: authoritative `777`, fallback `38`, unsupported-shape `0`

## Runtime And Proof Surfaces

The key runtime work landed in:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
- [LookupExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx)

The completed pass widened the current seam by adding:

- recursion-safe referenced-formula source materialization
- delegated evaluation rescue for referenced formula-backed sources
- scalarization of `1x1` matrix lookup inputs
- earlier scalar-error rejection on lookup-family inputs
- formula-backed single-cell lookup result projection via the same
  referenced-formula materialization bridge

The targeted proof surfaces are:

- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)
- [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)
- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)

Those buckets now prove:

- bounded helper support for formula-backed lookup result vectors
- bounded helper support for one-argument `2D` formula-backed `LOOKUP`
  result projection
- bounded helper support for named `VLOOKUP` and scalar `INDEX` on the same
  seam
- live authoritative-with-fallback carry-through on the widened seam
- Calc-backed replay-corpus stats with the improved lookup-residual
  baseline

## Interpretation

The important positive result is that the old lookup-residual shape problem
is now largely gone.

The corpus now records:

- a major `LOOKUP` authoritative increase
- a major reduction in total fallback
- a major reduction in unsupported-shape fallout
- a moderate reduction in residual mismatch

The important retained boundary is now concentrated on:

- `LOOKUP` scalar-projection parity rows
- `VLOOKUP` / `XLOOKUP` scalar host-surface rows
- bounded `INDEX` host-surface rows

So this evidence supports a strong mixed-result closeout:

- major runtime conversion success on the original lookup-shape bottleneck
- real overall seam improvement
- incomplete host-surface and scalar-parity cleanup

## Validation

The following validation passed for this wave:

- `CPPUNIT_TEST_NAME=testInterpretTailEngineEvaluatorHelper CppunitTest_sc_ucalc_shared_cases`
- `CPPUNIT_TEST_NAME=testInterpretTailEngineEvaluatorAuthoritativeWithFallback CppunitTest_sc_ucalc_formula2`
- `SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_STATS=1 SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_PROBE_DIAGNOSTICS=1 SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_PROBE_DIAGNOSTIC_LIMIT=400 CPPUNIT_TEST_NAME=testAuthorityStats CppunitTest_sc_interpret_tail_corpus`
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
