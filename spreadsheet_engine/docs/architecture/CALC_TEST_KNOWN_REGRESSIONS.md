# Calc Test Known Regressions

Status: active gating list — known-broken tests that must be cleared as a
standalone effort before they can be removed from this list

## Purpose

This list documents `sc_ucalc_formula2` test cases that are known to fail on
the current `calc_extraction` branch state. The failures accumulated across
the rename-gaming, lambda-relocation, compat-relocation, and
wrapper-retirement episodes (see initiative doc), and predate the
RPN-evaluator subsystem work.

The list is the **baseline** the project accepts in order to proceed with
Batch 2+ admissions of the RPN evaluator initiative. New work must not
increase the failure count beyond this list. A separate older-failures
cleanup workstream is responsible for shrinking the list back to zero.

## Verification command

The wrapper script
[run_known_regression_gate.sh](/home/ubuntu/repos/libreoffice/spreadsheet_engine/integration/libreoffice/run_known_regression_gate.sh)
runs the test and exits non-zero only if the failure set differs from this
list (i.e., a *new* regression slipped in, or an old one was *unintentionally*
fixed without the list being updated).

## Failure baseline (2026-04-19, branch tip after `0e94cc217`)

34 tests fail in `CppunitTest_sc_ucalc_formula2`. Total run: 125 tests.

### External reference (2)

- `testExternalRefFunctions`
- `testExternalRefUnresolved`

### Recalc / dependency tracking (5)

- `testFormulaDepTracking`
- `testFormulaDepTracking3`
- `testFormulaDepTrackingDeleteCol`
- `testFormulaDepTrackingDeleteRow`
- `testIterations`

### Function evaluation (18)

Likely root cause: legacy fallback paths regressed during retirement +
relocation episodes; recalc/observe interaction with the seam returns wrong
values or false `Err:522` (Circular Reference) on dependency change.

- `testFuncCHITEST`
- `testFuncCHOOSE`
- `testFuncDATEDIF`
- `testFuncFTEST`
- `testFuncFTESTBug`
- `testFuncGCD`
- `testFuncIF`
- `testFuncLCM`
- `testFuncMATCH`
- `testFuncNOW`
- `testFuncRefListArraySUBTOTAL`
- `testFuncRowsHidden`
- `testFuncSHEET`
- `testFuncSUMSQ`
- `testFuncSUMX2MY2`
- `testFuncSUMX2PY2`
- `testFuncTTEST`
- `testFuncTableRef`

### InterpretTail engine evaluator (3)

These directly exercise the seam. Their failure suggests the seam's
authoritative-with-fallback / math-scalar / statistical-distribution
paths regressed.

- `testInterpretTailEngineEvaluatorAuthoritativeWithFallback`
- `testInterpretTailEngineEvaluatorMathScalarAuthoritative`
- `testInterpretTailEngineEvaluatorStatisticalDistributionAuthoritative`

### Specific tdf bugs (3)

- `testTdf93415`
- `testTdf147398`
- `testTdf156985`

### Shell / shared / coercion (3)

- `testModernLogicalNameShellPhase7`
- `testReferenceShapePhase5`
- `testSharedStatisticalDelegations`

## Bisect notes (partial)

| Commit | Failures | Notes |
|---|---|---|
| `1887af02d` | 2 | ~100 commits back — near-green baseline |
| `137b85604` | 16 | "relocate legacy statistical wrapper surface" |
| `f9fdfa641` | 16 | flat through several wrapper-retirement docs |
| `070b33d5f` | (unverified) | "restore wrapper names for honest metrics" — large rename revert |
| `d02eb8b4b` | 30 | "retire helper-backed legacy wrappers" — +14 failures |
| `3e4faa939` | 31 | "retire legacy date family wrappers" — +1 |
| `d48af0946` | 31 | "retire scalar statistical wrappers" — flat |
| `164b89009` | 41 | "sub-100 lambda surface" — +10 |
| `98a5aef43` | 40 | last pushed — fixed 1 incidentally |
| `0e94cc217` (current) | 40 | TRUE/FALSE recalc regression fixed (covered separately) |

The cleanup workstream should bisect each cluster to its specific
introduction commit and restore the affected behavior without un-doing the
retirement that introduced the regression. Some root causes will be in the
seam's caching/observe behavior interacting with the legacy fallback path;
those are the hardest.

## Removal protocol

When a failure is fixed:
1. Run `CppunitTest_sc_ucalc_formula2` and confirm the test passes.
2. Remove its name from this list.
3. Update the failure count at the top of this doc and in
   `PROJECT_STATUS.md`.
4. The wrapper script will then start treating any reappearance as a
   regression.
