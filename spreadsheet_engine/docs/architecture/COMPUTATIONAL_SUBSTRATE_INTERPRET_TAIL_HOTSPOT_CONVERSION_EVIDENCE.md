# Computational Substrate InterpretTail Hotspot Conversion Evidence

Status: completed evidence summary for the focused InterpretTail
hotspot-conversion pass

## Evidence Summary

This pass closed with:

- real runtime widening on the already-admitted live `InterpretTail ->
  engine` seam
- a strong `DATEVALUE` parity win
- a moderate overall fallback and mismatch reduction
- only a small reduction in unsupported-shape fallout
- a still-dominant residual `LOOKUP` hotspot rather than a full lookup
  conversion win

## Frozen Baseline Versus Completed Snapshot

Frozen starting point:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1126`
- `interpret_tail_authoritative_fallback_total=362`
- `interpret_tail_fallback_unsupported_formula_shape=243`
- `interpret_tail_fallback_shadow_mismatch=97`
- `interpret_tail_fallback_unsupported_host_surface=22`

Completed rerun:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1156`
- `interpret_tail_authoritative_fallback_total=332`
- `interpret_tail_fallback_unsupported_formula_shape=241`
- `interpret_tail_fallback_shadow_mismatch=72`
- `interpret_tail_fallback_unsupported_host_surface=19`

Net change:

- probe-covered promoted-family cells: `+0`
- authoritative routes: `+30`
- total fallbacks: `-30`
- unsupported-shape fallbacks: `-2`
- shadow mismatches: `-25`
- unsupported host surface: `-3`

## Per-Family Completed Snapshot

Per promoted family, the completed corpus snapshot is:

- `VALUE`: authoritative `14`, fallback `1`, attempts `15`
- `DATEVALUE`: authoritative `29`, fallback `2`, attempts `31`
- `TIMEVALUE`: authoritative `8`, fallback `0`, attempts `8`
- `NUMBERVALUE`: authoritative `9`, fallback `0`, attempts `9`
- `MATCH`: authoritative `102`, fallback `15`, attempts `117`
- `XMATCH`: authoritative `24`, fallback `9`, attempts `33`
- `LOOKUP`: authoritative `555`, fallback `260`, attempts `815`
- `VLOOKUP`: authoritative `296`, fallback `17`, attempts `313`
- `HLOOKUP`: authoritative `39`, fallback `0`, attempts `39`
- `XLOOKUP`: authoritative `78`, fallback `19`, attempts `97`
- `INDEX`: authoritative `2`, fallback `9`, attempts `11`

The main retained per-family contributors are now:

- `LOOKUP`: `215` unsupported-shape fallbacks and `45` shadow mismatches
- `VLOOKUP`: `8` unsupported-host-surface fallbacks and `7` shadow
  mismatches
- `XLOOKUP`: `6` unsupported-host-surface fallbacks and `10` shadow
  mismatches
- `INDEX`: `4` unsupported-shape fallbacks and `5`
  unsupported-host-surface fallbacks
- `MATCH`: `9` unsupported-shape fallbacks and `6` shadow mismatches

The most important hotspot change is `DATEVALUE`:

- before: authoritative `4`, fallback `27`, shadow mismatch `27`
- after: authoritative `29`, fallback `2`, shadow mismatch `2`

## Runtime And Proof Surfaces

The key runtime work landed in:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)

The completed pass widened the current seam by adding:

- bounded standalone parse rescue for `VALUE`, `DATEVALUE`, and `TIMEVALUE`
- bounded referenced-formula source materialization for
  `BASISODATETIME(...)`-shaped source cells
- bounded matrix materialization from references and names
- bounded matrix binary-operation materialization for scalar-preserving
  lookup requests
- scalar projection of empty lookup results that still close exactly

The targeted proof surfaces are:

- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)
- [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)
- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)

Those buckets now prove:

- bounded helper support for month-name `DATEVALUE` parsing
- bounded helper support for referenced `BASISODATETIME(...)` date materialization
- bounded helper support for matrix-arithmetic `LOOKUP` shapes
- live authoritative-with-fallback carry-through on the widened seam
- Calc-backed replay-corpus stats with the improved hotspot baseline

## Interpretation

The important positive result is that the current delegated evaluator seam
now closes much more of its existing date-parsing surface.

The corpus now records:

- a major `DATEVALUE` parity improvement
- lower total fallback
- lower total mismatch
- elimination of the small prior `LOOKUP` host-surface residue

The important negative result is that the pass did not materially shrink the
largest `LOOKUP` unsupported-shape or mismatch bucket.

So this evidence supports a mixed-result closeout:

- strong hotspot win for `DATEVALUE`
- real overall seam improvement
- incomplete `LOOKUP` conversion

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
