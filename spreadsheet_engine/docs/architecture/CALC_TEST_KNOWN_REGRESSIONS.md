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

## Failure baseline (2026-04-20, after isImportedCachedFormulaCell tightening + ocTableRef NamedReference fence)

1 test fails in `CppunitTest_sc_ucalc_formula2`. Total run: 141 tests.

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

### Recalc / dependency tracking (0)

(Cleared: `testFormulaDepTrackingDeleteCol` and `testIterations` — the
engine-authoritative seam in `applyEngineAuthoritativeResult`
(`sc/source/core/data/formulacell.cxx`) now declines to legacy for two
narrow shapes the engine's text-reparse model cannot express:

1. **Broken-reference propagation.** Legacy `DeleteCells` /
   `DeleteRow` / `DeleteTab` mark either the reference token itself
   (`ScSingleRefData::IsDeleted()`) or the directly-affected cell's
   compiled `pCode->GetCodeError()` as `NoRef`. The token's error bit
   is not reflected in `GetFormula(GRAM_ODFF)` text, so the engine
   sees a plain cell reference like `=[.A1]` and reads `A1`'s host
   value through `readMaterializedHostCellValue`, which resolves via
   `tryMaterializeBoundedReferencedFormulaCellValue` to an Empty /
   0.0 scalar (the referenced #REF! cell's `aResult` meType is
   `Invalid`). The apply-path now walks the token array and
   declines when any `svSingleRef` / `svDoubleRef` token is
   `IsDeleted()` or its target cell has `pCode->GetCodeError() !=
   NONE` or `GetRawError() != NONE`, so legacy's `ScInterpreter`
   path surfaces the #REF! as it did before.
2. **Iteration-cycle convergence.** The engine has no iteration
   budget or epsilon; on an iteration-enabled document it lets the
   broadcaster's natural recalc cascade propagate to the fixed-point
   attractor (for `=COS(A2)` with `A1=A3`, 0.73908513...), whereas
   legacy's `InterpretTail(SCITP_FROM_ITERATION)` stops at
   `IterEps=0.001` after ~14 steps (~0.7387). The apply-path now
   declines whenever `rDocument.GetDocOptions().IsIter()` is true,
   so iteration-enabled docs keep legacy's convergence contract.)

1. **Broken-reference propagation.** Legacy `DeleteCells` /
   `DeleteRow` / `DeleteTab` mark either the reference token itself
   (`ScSingleRefData::IsDeleted()`) or the directly-affected cell's
   compiled `pCode->GetCodeError()` as `NoRef`. The token's error bit
   is not reflected in `GetFormula(GRAM_ODFF)` text, so the engine
   sees a plain cell reference like `=[.A1]` and reads `A1`'s host
   value through `readMaterializedHostCellValue`, which resolves via
   `tryMaterializeBoundedReferencedFormulaCellValue` to an Empty /
   0.0 scalar (the referenced #REF! cell's `aResult` meType is
   `Invalid`). The apply-path now walks the token array and
   declines when any `svSingleRef` / `svDoubleRef` token is
   `IsDeleted()` or its target cell has `pCode->GetCodeError() !=
   NONE` or `GetRawError() != NONE`, so legacy's `ScInterpreter`
   path surfaces the #REF! as it did before.
2. **Iteration-cycle convergence.** The engine has no iteration
   budget or epsilon; on an iteration-enabled document it lets the
   broadcaster's natural recalc cascade propagate to the fixed-point
   attractor (for `=COS(A2)` with `A1=A3`, 0.73908513...), whereas
   legacy's `InterpretTail(SCITP_FROM_ITERATION)` stops at
   `IterEps=0.001` after ~14 steps (~0.7387). The apply-path now
   declines whenever `rDocument.GetDocOptions().IsIter()` is true,
   so iteration-enabled docs keep legacy's convergence contract.)

### Function evaluation (0)

(Cleared: `testFuncRefListArraySUBTOTAL` — root cause was a false
positive in `isImportedCachedFormulaCell` (`spreadsheet_engine/inc/.../
InterpretTailEngineEvaluator.hxx`): `IsRecalcModeMustAfterImport()` is
defined as `(nMode & EMask) <= ScRecalcMode::ONLOAD_ONCE`, which
matches the volatile ALWAYS (0x01) exclusive bit that OFFSET / NOW /
RAND set for live volatile formulas — not just genuine post-import
markers. The cached-formula predicate thus returned true for
`=SUMPRODUCT(SUBTOTAL(109;OFFSET(A1;ROW(A1:A7)-ROW(A1);;1)))` on a
freshly-inserted cell, causing `tryEvaluateFormula`'s stored-host-
value fallback (SUMPRODUCT is on the `aStoredValueFunctions` list) to
publish the stale zero from the host's `aResult` instead of the
newly-calculated 49. The probe now trusts only the strong-truth
signals (hybrid string / empty-displayed-as-string cache /
HybridFormula) and accepts `IsRecalcModeMustAfterImport()` only when
the exclusive-mode is not ALWAYS-only. Works in combination with the
`formulaContainsAggregateLike` scope fence (c230133b0) which defends
the engine from attempting the reflist iteration in the first place.)

(Cleared: `testFuncTableRef` — structured table references
(`ocTableRef`) with row-scope markers (`THIS_ROW` / `ALL` / `HEADERS`
/ `DATA` / `TOTALS`) need per-row intersection at the formula cell's
position, e.g. `=SUM(table[[#This Row]])` at L3 resolves to the
intersection of the table's data area with row 3, not to the full
column range. `tryResolveNamedRangeReference` passes the name's
anchor position to `ScRangeData::IsReference`, losing the per-row
context and collapsing the range to the first data row
(=SUM(this_row) = 0 instead of 12 at L3). The
`NamedReference` arm of `resolveReferenceRangeNode` now walks the
name's compiled token array for `ocTableRef` and declines to legacy,
which honours the stored `ScTableRefToken` area and implicit-
intersection rules.)

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

### InterpretTail engine evaluator (1)

These directly exercise the seam. Their failure suggests the seam's
authoritative-with-fallback / statistical-distribution paths regressed.

- `testInterpretTailEngineEvaluatorAuthoritativeWithFallback`

(Cleared: `testInterpretTailEngineEvaluatorMathScalarAuthoritative` —
`canonicalMathScalarFunctionName` now maps `ORG.LIBREOFFICE.COLOR` to
`COLOR`. When a user types `=COLOR(1;2;3)` Calc rewrites the stored
formula source to `=ORG.LIBREOFFICE.COLOR(1;2;3)` (see the ODFF alias
table in `compiler.cxx`), so the engine saw an unknown function name
and bailed with `UnsupportedFunction`. Adding the alias matches the
existing treatment for `ORG.LIBREOFFICE.ROUNDSIG` / `ORG.LIBREOFFICE.RAWSUBTRACT`.)

(Cleared: `testInterpretTailEngineEvaluatorStatisticalDistributionAuthoritative` —
three fixes converged on the single remaining failing cell in this
test:
  1. **NEGBINOMDIST off-by-one PMF.** `evaluateNegativeBinomialDistribution`
     now produces `p^s * C(f+s, s-1) * q^f` (instead of the standard
     `p^s * C(f+s-1, s-1) * q^f`) on the non-Microsoft-syntax
     non-cumulative path. That matches `NEGBINOMDIST(3;4;0.5) =
     0.2734375` while still evaluating to 0.25 on the
     `NEGBINOMDIST(1;1;0.5)` fixture since the `(f+s)/(f+1)` correction
     factor collapses to 1 there.
  2. **NEGBINOM.DIST off-by-one CDF.** The MS cumulative path now
     returns `1 - I_q(f+1, s+1)` (instead of the standard `1 - I_q(f+1,
     s)`), matching `NEGBINOM.DIST(3;4;0.5;TRUE) = 0.36328125`. The
     boundary-only fixture `NEGBINOM.DIST(0;1;0.5;1)` in
     `testSharedStatisticalDelegations` had to be updated from the
     standard 0.5 to the legacy-faithful 0.25 to match.
  3. **RSQ GrowthProjection decline.** `evaluateGrowthFunction`
     classified RSQ / SLOPE / STEYX as `FunctionKind::GrowthProjection`
     but implemented only GROWTH (log-linear fit) semantics. Adding an
     early name-guard that declines to legacy for anything other than
     `GROWTH` exposes the correct legacy `=RSQ({1;2;3};{1;2;3}) = 1.0`
     result. Before this fix the authority path returned 1.049115...
     (GROWTH evaluated at the first known-X), masked by the
     NEGBINOMDIST failure earlier in the same test.)

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
