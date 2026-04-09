# Computational Substrate InterpretTail Engine Evaluator Switchover Evidence

Status: completed evidence for the first live evaluator switchover pass

## Landed Runtime Surface

The runtime switchover seam landed in:

- [../../../sc/source/core/data/formulacell.cxx](../../../sc/source/core/data/formulacell.cxx)
- [../../../spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx](../../../spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)

That seam now:

- resolves the live rollout mode from
  `SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR`
- classifies the first delegated family directly inside
  `ScFormulaCell::InterpretTail`
- runs engine-backed observe and shadow modes in AutoCalc sessions
- applies engine results authoritatively with explicit fallback when the
  routed family is supported

## Delegated Family

The landed delegated family is:

- literal-only `VALUE`
- literal-only `DATEVALUE`
- literal-only `TIMEVALUE`
- literal-only `NUMBERVALUE`

The current fallback reasons include:

- unsupported tail context
- unsupported formula shape
- unsupported function
- parse failure
- projection failure
- shadow mismatch

The current mismatch reasons include:

- error mismatch
- numeric-value mismatch
- format-type mismatch

## Proof

Helper and bounded family proof is in:

- [../../../sc/qa/unit/ucalc_shared_cases.cxx](../../../sc/qa/unit/ucalc_shared_cases.cxx)

Live AutoCalc observe, shadow, authoritative-route, and fallback proof is in:

- [../../../sc/qa/unit/ucalc_formula2.cxx](../../../sc/qa/unit/ucalc_formula2.cxx)

Those live buckets prove:

- `observe` support classification on a real AutoCalc `InterpretTail` path
- `shadow` comparison on a real AutoCalc `InterpretTail` path
- `authority` bypass and direct result projection on a supported formula
- explicit Calc fallback on an unsupported formula shape

## Runtime Outcome

This pass is not “proof only.” It landed a real production routing seam.

The important runtime result is:

- supported formulas in the landed family no longer require `ScInterpreter`
  in `authority` mode

That is the first real `InterpretTail -> engine` bypass in this program.

## Validation

Validated in this pass:

- `testInterpretTailEngineEvaluatorHelper`
- `testInterpretTailEngineEvaluatorObserveAndShadowCompare`
- `testInterpretTailEngineEvaluatorAuthoritativeWithFallback`

Full closeout validation also includes:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`
