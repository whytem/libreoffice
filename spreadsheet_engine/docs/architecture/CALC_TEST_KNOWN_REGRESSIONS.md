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

## Failure baseline (2026-04-19, after IF matrix-frame fence fix)

6 tests fail in `CppunitTest_sc_ucalc_formula2`. Total run: 135 tests.

Previous baseline was 30 tests. Progress so far:

- **Stored-host-value-truth self-referential gate (30 → 18, -12)**: gated the
  `importedRootUsesStoredHostValueTruth` fallback behind imported-canonical /
  imported-cached-formula predicates. The fallback was firing in
  live-recalc (e.g. `SetString` + `CalcFormulaTree`) contexts where the
  "host value" is the previous result of the *same* cell we are currently
  recomputing. Returning that stale value as authoritative left formulas
  like `=SHEETS()`, `=IF(...)`, `=FTEST(...)`, etc. frozen at zero after
  the first recalc that touched them.
- **ScCount/ScCount2 wrapper retirement (18 → 17, -1)**: inlining
  `IterateParameters(ifCOUNT)` at the compat-dispatch call sites
  incidentally cleared `testFuncSUMSQ` on the first unrelated SUMSQ edge
  case.
- **NumericAggregate engine surface fixes (17 → 14, -3)**:
  - Scalar-text argument in SUM / SUMSQ / AVERAGE / PRODUCT / DEVSQ now
    surfaces #VALUE! (NoValue) instead of Err:502 (IllegalArgument),
    matching legacy `=SUMSQ("a";1;-4;2)` error semantics.
  - `SUMX2MY2 / SUMX2PY2 / SUMXMY2` now walk the two matrix operands in
    lockstep, skipping a pair only when *either* matrix has a non-numeric
    cell, matching legacy `CalculateSumX2MY2SumX2DY2` pairing. Previously
    the engine collected each matrix independently, which desynced pair
    ordering when empty cells appeared asymmetrically between X and Y
    ranges.
  - Arity mismatch (non-two parameters) declines to legacy so that
    legacy `MustHaveParamCount(2, 2)` still surfaces Err:511
    (ParameterExpected); the engine's api::Error enum doesn't model
    ParameterExpected, and we must not flatten it to IllegalArgument.
- **GCD / LCM argument-shape error semantics (15 → 13, -2)**:
  `collectNumericArguments` now classifies the argument node and
  dispatches text and empty cells differently for each:
    - Multi-cell range refs skip text and empty silently (legacy
      `ScValueIterator`).
    - Inline array constants surface `Err:502` on text / empty
      (legacy `CalcGcdLcm` matrix walk).
    - Single-cell references coerce empty to 0.0 and let scalar text
      coercion raise `#VALUE!` (legacy `GetDouble`).
  Also corrected the empty-values fallthrough: `GCD` now returns 0 and
  `LCM` returns 1 when the collected vector is empty, matching the
  initial accumulator in `pushLegacyGcdOrLcm`.
- **Volatile-listener retention in engine-authoritative path (13 → 12, -1)**:
  `applyEngineAuthoritativeResult` in `formulacell.cxx` was unconditionally
  ending `BCA_LISTEN_ALWAYS` and `SetExclusiveRecalcModeNormal` after
  applying the engine's result, even when the formula contained NOW() /
  TODAY() / RAND() / a volatile macro. Legacy `Interpret()` only clears
  these when `VolatileType == NOT_VOLATILE` after running the formula.
  For short-circuited IFs like `=IF(A1>0;NOW();0)` the engine may
  evaluate the FALSE branch (returning 0) without executing `NOW()`,
  so the volatile flag is still warranted. Now we keep listeners and
  keep the formula in the formula tree whenever `IsRecalcModeAlways()`
  is set, and defer the mode drop to a legacy run. Clears
  `testFuncNOW`.

### External reference (0)

All two previous failures cleared by the stored-host-truth fix (imports
now correctly consume the cached value; live calc no longer shadows it).

### Recalc / dependency tracking (2)

- `testFormulaDepTrackingDeleteCol`
- `testIterations`

### Function evaluation (2)

Likely root cause: legacy fallback paths regressed during retirement +
relocation episodes; recalc/observe interaction with the seam returns wrong
values or false `Err:522` (Circular Reference) on dependency change.

- `testFuncRefListArraySUBTOTAL`
- `testFuncTableRef`

(Cleared: `testFuncIF` — `tryPlanEngineIfJump` and `tryPlanEngineIfError`
in `interpr4.cxx` lacked a JumpMatrix-on-stack scope fence. The matrix
formula `=IF({1;0};IF(1;23);42)` makes the outer `IF` build a JumpMatrix
on the stack; for each per-cell iteration the inner `IF(1;23)` is then
dispatched with that JumpMatrix sitting immediately below the scalar
condition `1`. Legacy `ScIfJump` runs `MatrixJumpConditionToMatrix`
first, which (when `IsInArrayContext()` and `GetStackType(2) ==
svJumpMatrix`) coerces the scalar condition to a 1x1 matrix and creates
a nested JumpMatrix so the outer iteration receives a matrix-shaped
result for the current cell. The engine's scalar fast path skipped that
conversion and emitted a plain `aCode.Jump`, leaving the outer
JumpMatrix without a result for row 0 (Expected: 23, Actual: 0). Both
tryPlanEngine* lambdas now decline to legacy whenever
`GetStackType(2) == svJumpMatrix`, which preserves the matrix-frame
JumpMatrix protocol while still routing scalar IF/IFERROR/IFNA outside
matrix frames through `planIfBranch` / `planIfErrorBranch`.)

(Cleared: `testFuncMATCH` — `selookup::resolveMatchIndex` (the unified
runtime that both engine and legacy `ScMatchOp` now share) lacked the
trailing-empty trim that legacy `ScQueryCellIteratorDirect` enforced
implicitly. The horizontal MATCH formula `=MATCH(O2;A1:M1;1)` over data
filling only `A1:L1` left `M1` empty; in the linear text-lookup walk an
empty trailing cell (`compareFoldedText("", "Charlie") < 0`) extended
`oResolvedIndex` past the last real value `C` at column L (12), returning
13 instead of the expected 12. `resolveMatchIndex` now applies
`trimTrailingEmptyLookupLength` (matching the existing
`preserveTrailingEmptiesForExtendedMatch` carve-out for empty-lookup
queries) so trailing empties no longer absorb the resolved index.)

### InterpretTail engine evaluator (2)

These directly exercise the seam. Their failure suggests the seam's
authoritative-with-fallback / statistical-distribution paths regressed.

- `testInterpretTailEngineEvaluatorAuthoritativeWithFallback` (partial
  fix landed: `NORMSDIST` / `NORM.S.DIST` /
  `COM.MICROSOFT.NORM.S.DIST` / `LEGACY.NORMSDIST` now have a first-party
  handler in `evaluateStatisticalDistributionFunction` that reuses
  `evaluateNormalDistribution(x, 0, 1, cumulative)`. Previously the
  StatisticalDistribution classifier admitted the function but the
  dispatcher fell through to `UnsupportedFunction`, and with
  `isFamilyLocalDefaultOnFormula` routing the whole family through the
  authoritative-while-off path every `=NORM.S.DIST(x; TRUE())` /
  `=NORM.S.DIST(x; FALSE())` produced an authoritative fallback. The
  test still fails on downstream classification assertions for
  error-literal and scalar-expression roots now classifying as
  `ScalarRoot` rather than `Unknown`; those shifted when
  `isPromotableScalarRootNode` admitted ErrorLiteral / CellReference /
  BinaryOperation / UnaryOperation roots.)
- `testInterpretTailEngineEvaluatorStatisticalDistributionAuthoritative`

(Cleared: `testInterpretTailEngineEvaluatorMathScalarAuthoritative` —
`canonicalMathScalarFunctionName` now maps `ORG.LIBREOFFICE.COLOR` to
`COLOR`. When a user types `=COLOR(1;2;3)` Calc rewrites the stored
formula source to `=ORG.LIBREOFFICE.COLOR(1;2;3)` (see the ODFF alias
table in `compiler.cxx`), so the engine saw an unknown function name
and bailed with `UnsupportedFunction`. Adding the alias matches the
existing treatment for `ORG.LIBREOFFICE.ROUNDSIG` / `ORG.LIBREOFFICE.RAWSUBTRACT`.)

### Specific tdf bugs (0)

(Cleared: `testTdf156985` — seam SUM/SUMSQ/AVERAGE/DEVSQ aggregates now use
legacy `::KahanSum` (sc/inc/kahan.hxx) which queues the last non-zero summand
and snaps to exact 0.0 via `rtl::math::approxEqual` at `get()` time. The
engine's `spreadsheetengine::core::fp::KahanSum` is a plain Kahan-Babuška
accumulator and lacks the snap, so `SUM(-170.87, -223.73, -12.58, 234.98,
172.2)` returned ~-2.84e-14 instead of 0.0.)

### Shell / shared / coercion (0)

(All cleared.)

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
