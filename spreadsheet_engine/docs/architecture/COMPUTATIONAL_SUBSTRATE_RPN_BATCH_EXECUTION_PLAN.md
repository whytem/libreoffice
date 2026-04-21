# RPN Evaluator: Five-Batch Execution Plan

Status: active working plan for the RPN Evaluator Initiative

This document consolidates the current-state audit across the five remaining
subsystem batches and sequences their execution. It extends the scope policy
in [COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md](COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md)
with concrete per-batch member lists, substrate dependencies, and retirement
gates.

## Framing

Piece-by-piece migration continues to work for leaf functions whose engine
bodies already live in `semath::` / `sefinance::` / `sestat::`. It no longer
works for the remaining ~51 `Sc*` methods + ~25 `pushLegacy*` lambdas,
because they share machinery — porting them one at a time means rebuilding
that machinery five or more times.

The remaining work splits into five subsystem batches plus one parallel
wrapper-cleanup lane. Each batch owns a single substrate header under
`spreadsheetengine/runtime/` and lands in four phases:

1. **Substrate** — a new engine header defining the decision/operand types
   the batch needs. No opcode routes through it. Unit tests only.
2. **Admission** — Calc dispatch cases become `try-engine-then-fallback`,
   reading the engine substrate. Counters start moving.
3. **Parity** — corpus runs validate acceptance rate. Decline shapes are
   either documented as intentional or fixed. Deliberate-decline tests
   exercise the decline counters.
4. **Retirement** — per the template in the initiative doc: engine path is
   authoritative, legacy body + `pushLegacy*` lambda deleted, fallback
   replaced with `OSL_FAIL` + engine-consistent error.

## Dependency DAG

```
Batch 1 Control Flow ───┐
                        ├──► Batch 3 Criteria/DB
Batch 2 Reference Ops ──┤
                        └──► Batch 4 Matrix-Native ──► Batch 5 Dynamic Array
```

Batch 1 has no upstream dependency and can land in parallel with Batch 2.
Batch 3 needs reference resolution (named ranges, database ranges) from
Batch 2. Batch 4 needs matrix-operand materialization that begins in Batch 2
and widens here. Batch 5 extends Batch 4 with spill-range semantics.

## Batch 1: Control Flow

Substrate: `spreadsheetengine/runtime/RpnControlFlow.hxx` — **landed**

Members (7):

| Opcode | Location | Difficulty |
|---|---|---|
| `ocIf` / `ScIfJump` | `interpr1.cxx:459` | Medium (scalar path simple; matrix path needs Batch 4) |
| `ocIfJumpNotMatrix` / `ScIfJumpNotMatrix` | `interpr1.cxx:509` | Low |
| `ocChoose` / `ScChooseJump` | `interpr1.cxx:538` | Medium |
| `ocIfs_MS` / `pushLegacyIfs` | `interpr4.cxx:7777` | Low |
| `ocSwitch_MS` / `pushLegacySwitch` | `interpr4.cxx:7833` | High (cell-deref + external-ref cases) |
| `ocIfError` / `pushLegacyIfError` | `interpr4.cxx:7643` | High (matrix-aware) |
| `ocIfNA` / `pushLegacyIfNA` | shares `pushLegacyIfError` body | High |
| `ocLet` / `ScLet` | `interpr1.cxx:6189` | High (nested interpreter spawn) |

Engine-side reusable helpers already present:
- `api::logic::selectIfBranch`, `chooseJumpIndex`, `selectIfErrorAction`,
  `evaluateIfsCondition`, `normalizeChooseIndex`
- `sejumpexec::initializeChooseJumpMatrix`, `initializeIfErrorJumpMatrix`
- `seswitchexec::makeNumericSwitchValue`, `makeTextSwitchValue`, `matchesSwitchCase`
- `seletexec::replaceNamesToResult`, `copyTokenSlice`

First admission target: `ocIf` scalar condition.
Scope fence: matrix conditions stay on legacy until Batch 4.

Critical concern: `aCode.Jump()` / `FormulaTokenIterator` is Calc-host PC
mutation with no engine equivalent. The substrate header intentionally
returns `BranchPlan` descriptors; the caller (Calc) still advances PC.
Full engine ownership of the RPN loop is a later initiative.

## Batch 2: Reference Ops

Substrate: `spreadsheetengine/runtime/RpnReference.hxx` — **landed**

Members (12):

| Function | Location | Difficulty |
|---|---|---|
| `ScColumn`, `ScRow`, `ScSheet` | `interpr1.cxx:2671` / `2803` / `2934` | Medium (scalar vs matrix-axis dual path) |
| `ScColumns`, `ScRows`, `ScSheets` | `interpr1.cxx:2495` / `2556` / `2617` | Low |
| `ScMultiArea`, `ScAreas` | `interpr1.cxx:7681` / `7694` | Low |
| `ScAddressFunc` | `interpr1.cxx:7237` | Low |
| `ScIndirect` | `interpr1.cxx:7183` | Low (engine helper `seindirectexec::resolveIndirectReference` exists) |
| `ScOffset` | `interpr1.cxx:7323` | Medium |
| `ScIndex` | `interpr1.cxx:7487` | High (matrix-returning form) |

Required host-facade extensions:
- `resolveName(StringView, CellAddress)` → `ResolvedReference`
- `resolveDatabase(StringView)` → `DatabaseMetadata`
- `resolveExternalRef(FileId, SheetName, CellRange)` → `ResolvedReference`
- `iterateRangeWithPosition(ResolvedReference)` → lazy walker

Retirement in two commits:
- A: axis/count members (8) with clean `100%` audit acceptance
- B: shape-defer-aware members (`ScIndirect`, `ScOffset`, `ScIndex`,
  `ScAddressFunc`) after decline paths produce explicit `FormulaError::NoRef`

Critical concern: `ScIndex` matrix-returning form requires immediate
materialization to a dense 2D array. `RpnValue`'s `Matrix` kind currently
defers via `NeedsMatrixMaterialization`; Batch 2 must define the host
materialization contract for INDEX's matrix-return shape before retirement.

External-ref handling is scope-fenced to decline on first admission.

## Batch 3: Criteria / Database

Substrate: `spreadsheetengine/runtime/RpnCriteria.hxx`,
`spreadsheetengine/runtime/RpnDatabase.hxx` — **landed**

Members (21):

Criteria (9):
- `ScCountIf` (`interpr1.cxx:3535` — largest, 207 lines)
- `ScSumIf`, `ScAverageIf` (inline dispatchers → `IterateParametersIf`)
- `ScCountIfs`, `ScSumIfs`, `ScAverageIfs` (inline → `IterateParametersIfs`)
- `ScMinIfs_MS`, `ScMaxIfs_MS` (inline → `IterateParametersIfs`)
- `ScCountEmptyCells` (`interpr1.cxx:3030` — no criteria, empty-cell scan)

Database (12):
- `ScDBSum`, `ScDBAverage`, `ScDBMax`, `ScDBMin`, `ScDBProduct` — all
  delegate to `DBIterator` (trivial)
- `ScDBCount`, `ScDBCount2` — dual iterator pattern
- `ScDBGet` (`interpr4.cxx:2922`) — uniqueness-enforcing GET
- `ScDBStdDev`, `ScDBStdDevP`, `ScDBVar`, `ScDBVarP` — via `GetDBStVarParams`

Shared Calc helpers to migrate:
- `IterateParametersIf()` — ~630 lines, single-criterion SUMIF/AVERAGEIF
- `IterateParametersIfs()` — ~500 lines, multi-criteria Ifs family with
  range-reduce optimization and condition vector caching
- `GetDBParams()` — ~135 lines, DB range resolution + field mapping
- `DBIterator()` — ~100 lines, aggregate dispatcher
- `GetDBStVarParams()` — variance accumulator

Engine-side partial coverage:
- `QueryRuntime.hxx` already provides `matchesWholeCellLookupText`,
  `matchesQueryText`, `makeCriteriaPredicate`, `matchesCriteriaPredicate`,
  `evaluateCriteriaAggregate` (single-aggregate, single-range, vector of
  predicates)
- Missing: multi-range IFS orchestration, database query descriptor,
  variance/stddev (delegates to `semath::`)

Required host-facade extensions:
- `getRegexMode()` → `bool` (ScCalcConfig wildcard/regex)
- `iterateRangeWithCriteria(ResolvedReference, Predicate)`
- `getDatabase(StringView)` → header + data range + filter state
- `getHiddenRowStatus(RowIndex, SheetId)` (for SUBTOTAL)

Critical concern: `IterateParametersIfs` range-reduce optimization shrinks
the main range to data bounds before evaluating criteria. The engine must
either replicate this (visible perf win on large ranges) or accept a
documented performance regression for first admission. Recommendation:
scope-fence the optimization to Batch 3 phase B.

## Batch 4: Matrix-Native

Substrate: `spreadsheetengine/runtime/RpnMatrix.hxx` — **landed**

Members (17+):

Base matrix ops (11):
- `ScMatValue`, `ScMatRef`, `ScMatTrans`, `ScMatInv`, `ScMatMult`,
  `ScMatSequence`, `ScEMat`, `ScSumProduct`, `ScSumX2MY2`, `ScSumX2DY2`,
  `ScSumXMY2`

Regression/forecast (6+):
- `ScLinest`, `ScLogest`, `ScForecast`, `ScForecast_Ets`, `ScFourier`,
  `ScGrowth`, `ScTrend`

Engine-side primitives already present:
- `MatrixOperators.cxx`, `MathMatrix.hxx`, `MathStatistical.hxx` cover
  most numerical cores
- Gap: operand model + materialization contract

Required host-facade extensions:
- `materializeRangeToMatrix(ResolvedReference)` → `Matrix`
- `getCellFormat(CellAddress)` → `FormatIndex` (shared with Batch 3)
- `getEvaluatedCellSnapshot(CellAddress)` → value + format + array-formula flag

Retirement in three commits:
- A: base matrix ops (`ScMatTrans`, `ScMatInv`, `ScMatMult`, `ScMatValue`,
  `ScMatRef`, `ScMatSequence`, `ScEMat`)
- B: sum-product family (`ScSumProduct`, `ScSumX2MY2`, `ScSumX2DY2`, `ScSumXMY2`)
- C: regression/forecast (depends on matrix ops + sestat)

Critical concerns:
1. Three distinct numerical cores (LUP for `ScMatInv`, QR for Linest family,
   ETS state machine for `ScForecast_Ets`). Each has its own convergence,
   error handling, and mutation patterns.
2. `ScForecast_Ets` carries persistent state (`ScETSForecastCalculation`
   with `mpBase`, `mpTrend`, `mpPerIdx` arrays). First admission should
   decline the `ETSStatistics` variant.
3. Numerical parity — `MINVERSE` and `MMULT` may produce bit-different
   results due to summation order. Decide up-front whether to accept
   epsilon-tolerance or enforce legacy order.

Batch 4 retroactively unblocks Batch 1's `ScIfJumpNotMatrix` matrix
condition path. Plan to widen Batch 1 admission and retire
`ScIfJumpNotMatrix` as a C-stage commit in Batch 4.

## Batch 5: Dynamic-Array / Spill

Substrate: `spreadsheetengine/runtime/RpnSpill.hxx` — **landed**

Members (16):

- `ScFilter`, `ScSort`, `ScSortBy`, `ScUnique`
- `ScChooseCols`, `ScChooseRows`, `ScDrop`, `ScExpand`
- `ScHStack`, `ScVStack`, `ScTake`
- `ScTextSplit`, `ScToCol`, `ScToRow`
- `ScWrapCols`, `ScWrapRows`

Engine-side partial coverage:
- `api::Array` (`searray::`) namespace already provides geometry planners:
  `planTakeDropSlice`, `normalizeSelectionIndex`, `planChooseResultDimensions`,
  `planExpandDimensions`, `appendStackDimensions`, `planFlattenOutputDimensions`,
  `planWrapOutputDimensions`, `wrapDestination`
- Gap: spill-range allocation contract

Older inline implementations (`ScFilter`, `ScSort`, `ScSortBy`, `ScUnique`,
`ScTextSplit`) lack `searray::` helpers and need extraction before admission.

Required host-facade extensions (all hard-required):
- `allocateSpillRange(CellAddress, MatrixDimensions)` → `CellRange`
- `checkSpillCollision(CellRange)` → `bool`
- `getCurrentFormulaPosition()` → `CellAddress`
- `markArrayFormulaBounds(CellRange)` → void

Retirement in two commits:
- A: simple-shape (`ScSequence`, `ScSort`, `ScSortBy`, `ScUnique`,
  `ScFilter`, `ScTake`, `ScDrop`)
- B: shape-reshaping (`ScHStack`, `ScVStack`, `ScChooseCols`,
  `ScChooseRows`, `ScExpand`, `ScToCol`, `ScToRow`, `ScWrapCols`,
  `ScWrapRows`, `ScTextSplit`)

Critical concern: legacy returns a matrix token; no `#SPILL!` error emitted
by the member functions themselves. Engine must define spill lifecycle and
emit `#SPILL!` from the allocator on collision — this is new behavior, not
a migration of existing behavior.

## Parallel Lane: Wrapper Cleanup

Steady piecewise retirement for families already at engine parity:
- Covariance: `ScCovarianceP`, `ScCovarianceS`
- Hypothesis tests: `ScZTest`, `ScTTest`, `ScFTest`, `ScChiTest`
- Shape stats: `ScSkew`, `ScSkewp`, `ScAveDev`, `ScDevSq`
- Any remaining financial wrappers delegating to `sefinance::`
- Date-tail wrappers delegating to `sedatetime::`
- `ocText` / `ocCurrency` / `ocFixed` formatted-text tail

Target: ~20 retirements at ~1/week throughout the batch window.

## Explicit Non-Goals

The following remain `ScInterpreter::Sc*()` and do not enter any batch:
- External I/O: `ScExternal`, `ScMacro`, `ScDde`, `ScHyperLink`,
  `ScWebservice`, `ScFilterXML`, `ScEncodeURL`, `ScBahtText`,
  `ScGetPivotData`
- Non-deterministic clock/time policy: `ScCurrent`, `ScGetActDate`,
  `ScGetActTime`
- Host-integrated oddities: `ScTTT`, `ScDebugVar`, `ScNoName`, `ScBadName`
  (already retired), `ScMissing`, `ScStyle`, `ScColor`

These belong in Calc permanently. `ScRandom`, `ScRandbetween`, and
`ScRandArray` were later moved by the stack-machine relocation closeout via
`RuntimeEnvironment::sampleUniformReal()` plus engine-native random planners,
so they are no longer part of this non-goal set.

## Success Criteria (end of five batches)

- All five batch subsystems present under
  `spreadsheetengine/inc/spreadsheetengine/runtime/`
- `legacy_interpreter_subroutine_count` ≤ `40`
- `interp4_dispatch_legacy_lambda_count` ≤ `10`
- every retired opcode family shows non-zero exercised engine-first runtime on
  a focused or corpus audit lane before fallback deletion
- Acceptance rate ≥ `0.95` across that lane
- Live authoritative-match rate ≥ `99.5%`
- Host facade has stable explicit contracts for: address resolution,
  named/ext/DB range resolution, range iteration, regex mode, spill
  allocation, matrix materialization
- Every retirement commit follows the `ocBad` template

## Current Progress Against This Plan

- Batch 1 substrate and first scalar admissions: **landed**
- Batch 2 substrate and first scalar/reference admissions: **landed**
- Batch 3 substrate and first criteria/database admissions: **landed**
- Batch 4 substrate and first matrix admissions: **landed**
- Batch 5 substrate and simple-shape / shape-reshaping admissions: **landed**
- Parallel wrapper-cleanup: the focused pure text/info and
  parsing/inspection wave tracked in
  [../archive/authority_transfer/COMPUTATIONAL_SUBSTRATE_TEXT_INFO_RETIREMENT_PLAN.md](../archive/authority_transfer/COMPUTATIONAL_SUBSTRATE_TEXT_INFO_RETIREMENT_PLAN.md)
  is complete; the next remaining interpreter-resident cleanup target is the
  host-sensitive text tail
