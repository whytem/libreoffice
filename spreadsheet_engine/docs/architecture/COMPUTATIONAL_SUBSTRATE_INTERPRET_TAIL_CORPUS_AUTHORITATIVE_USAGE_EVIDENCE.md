# Computational Substrate InterpretTail Corpus Authoritative Usage Evidence

Status: completed evidence summary for the first non-zero authoritative
replay-corpus `InterpretTail` usage pass

## Evidence Summary

This pass closed with:

- a live seam source-bridge change
- new bounded seam diagnostics
- a stronger Calc-backed replay-corpus harness
- the first non-zero authoritative usage snapshot on the promoted live
  evaluator families

## Completed Corpus Snapshot

The completed Calc-backed corpus rerun now freezes:

- `interpret_tail_corpus_workbooks=500`
- `interpret_tail_corpus_formula_cells=50661`
- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=996`
- `interpret_tail_authoritative_fallback_total=395`
- `interpret_tail_fallback_parse_failure=0`
- `interpret_tail_fallback_unsupported_formula_shape=295`
- `interpret_tail_fallback_unsupported_host_surface=15`
- `interpret_tail_fallback_shadow_mismatch=85`

Per promoted family:

- `VALUE`: authoritative `13`, fallback `2`, attempts `15`
- `DATEVALUE`: authoritative `4`, fallback `27`, attempts `31`
- `TIMEVALUE`: authoritative `8`, fallback `0`, attempts `8`
- `NUMBERVALUE`: authoritative `9`, fallback `0`, attempts `9`
- `MATCH`: authoritative `91`, fallback `26`, attempts `117`
- `XMATCH`: authoritative `22`, fallback `11`, attempts `33`
- `LOOKUP`: authoritative `520`, fallback `295`, attempts `815`
- `VLOOKUP`: authoritative `291`, fallback `22`, attempts `313`
- `HLOOKUP`: authoritative `36`, fallback `3`, attempts `39`
- `INDEX`: authoritative `2`, fallback `9`, attempts `11`

## Targeted Proof

The key targeted proof buckets are:

- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)
  proves the live source-normalization helper path on bounded real examples
- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
  now proves:
  - bounded single-file corpus targeting
  - live hard recalc on a real `ScDocument`
  - live-source probe classification on the promoted function families
  - authoritative vs fallback accounting against corpus-expected values
- [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
  now feeds the live seam with `GRAM_ODFF`
- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
  now records bounded diagnostics for parse and shape failures

## Validation

The following validation passed for this wave:

- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_interpret_tail_corpus` with
  `SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_STATS=1`
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

## Practical Outcome

The pass gives the migration program a usable live-corpus metric.

Before this pass, the corpus answer was effectively:

- the seam is reached
- then it dies at parse failure

After this pass, the corpus answer is:

- the promoted families do achieve real authoritative usage
- the main remaining costs are now concentrated in unsupported-shape,
  mismatch, and host-surface boundaries

That is the evidence needed to steer the next evaluator wave intelligently.
