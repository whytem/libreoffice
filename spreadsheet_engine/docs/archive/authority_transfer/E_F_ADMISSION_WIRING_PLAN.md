# E/F Admission Wiring Plan

Detail for manual re-application of the Phase E (regression/forecast) and
Phase F (Batch 5 dynamic-array) admission dispatch-site changes to
`sc/source/core/tool/interpr4.cxx` on current main.

Verified against main at commit `6480ec3ed`. All line numbers are absolute
file offsets in current main and were verified with `grep -n`.

## 1. Current main-branch layout of `interpr4.cxx`

File length: ~14,308 lines.

Helper clusters (via `grep -n "tryPlanEngine"`):

| Cluster | Line range | Helpers |
|---|---|---|
| Control-flow (IfJump) | 7659–7765 | `tryPlanEngineIfJump` |
| Reference axis (Span / Axis) | 7767–8035 | `tryPlanEngineSpanCount`, `tryPlanEngineSingleCriterionAggregate`, `tryPlanEngineCountIf` |
| Empty-range + matrix | 8048–8719 | `tryPlanEngineCountEmptyCells`, `tryPlanEngineIdentityMatrix`, `tryPlanEngineSequenceMatrix`, `tryPlanEngineTranspose`, `tryPlanEngineMatrixDeterminant`, `tryPlanEngineMatrixMultiply`, `tryPlanEngineMatrixInverse` |
| DB family | 8729–9640 | `tryPlanEngineDatabaseAggregate`, `tryPlanEngineDatabaseVariance`, `tryPlanEngineDatabaseGet` |
| Multi-criterion + sum/avg if | 9647–9834 | `tryPlanEngineMultiCriterionAggregate`, `tryPlanEngineSumIf`, `tryPlanEngineAverageIf` |
| Indirect / Address | 9837–10023 | `tryPlanEngineIndirect`, `tryPlanEngineAddress` |
| Index / Offset / Areas / Axis | 10024–10430 | `tryPlanEngineIndex`, `tryPlanEngineOffset`, `tryPlanEngineAreaCount`, `tryPlanEngineAxisOrdinal` |
| Control flow 2 (Choose / IfError / Ifs / Switch) | 10432–11213 | `tryPlanEngineChooseJump`, `tryPlanEngineIfError`, `tryPlanEngineIfs`, `tryPlanEngineSwitch` |

Dispatch switch body starts around line 11215.

Current runtime-stats fields (`InterpreterDispatchRuntimeStatsStore`,
lines 336–359): `mnEngine*`, `mnRangeEngine*`, `mnControlFlowEngine*`,
`mnReferenceEngine*`, `mnCriteriaEngine*`, `mnMatrixEngine*`. **Absent**:
`mnSpillEngine*`, `mnRegressionEngine*`. Snapshot struct mirror in
`sc/source/core/inc/interpre.hxx` lines 73–96.

## 2. Insertion point for admission helpers

Both helper families go into the same slot: immediately after
`tryPlanEngineMatrixInverse` (closes line 8719), before the DB block
(begins line 8721). Keeps matrix/numeric-core helpers clustered together
and avoids touching intervening logic.

Recommended ordering in the new slot (insert at line 8720):

1. Phase E helpers (`tryPlanEngineLinestOrLogest`, `tryPlanEngineLinest`,
   `tryPlanEngineLogest`, `tryPlanEngineTrendOrGrowth`,
   `tryPlanEngineTrend`, `tryPlanEngineGrowth`, `tryPlanEngineForecast`,
   `tryPlanEngineFourier`)
2. Phase F helpers (`pushSpillEngineDecline`, `tryPlanEngineSpillSort`,
   `tryPlanEngineSpillSortBy`, `tryPlanEngineSpillFilter`,
   `tryPlanEngineSpillUnique`, `tryPlanEngineSpillTakeOrDrop`)

E uses existing `mnMatrixEngine*` counters. F introduces its own
`mnSpillEngine*` triple.

Both helper bodies can be lifted verbatim from the worktree diffs —
their bodies compile against the current main's
`convertMatrixRefToMatrixOperand` / `convertMatrixOperandToMatrixRef` /
`PushMatrix` / `PushDouble` / `PushError` / `selibreoffice::toFormulaError`
surface, which is unchanged since the worktree snapshot.

## 3. Dispatch-case wiring

### Phase E (lines 13456–13660 in current main)

| Opcode | Line | Current body | Target body |
|---|---|---|---|
| `ocTrend` | 13456 | `ScTrend();` | `if (!tryPlanEngineTrend()) ScTrend();` |
| `ocGrowth` | 13457–13460 | `warnIfLegacyGrowthProjectionReached(...); seinterpcompatdispatch::Dispatcher::growth(*this);` | `if (!tryPlanEngineGrowth()) { warnIfLegacyGrowthProjectionReached(...); seinterpcompatdispatch::Dispatcher::growth(*this); }` |
| `ocLinest` | 13461 | `ScLinest();` | `if (!tryPlanEngineLinest()) ScLinest();` |
| `ocLogest` | 13462 | `ScLogest();` | `if (!tryPlanEngineLogest()) ScLogest();` |
| `ocForecast_LIN` / `ocForecast` | 13463–13467 | `warnIfLegacyStatisticalDistributionReached(...); seinterpcompatdispatch::Dispatcher::forecast(*this);` | `if (!tryPlanEngineForecast()) { warnIfLegacyStatisticalDistributionReached(...); seinterpcompatdispatch::Dispatcher::forecast(*this); }` |
| `ocFourier` | 13660 | `ScFourier();` | `if (!tryPlanEngineFourier()) ScFourier();` |

Note: in current main, `ocGrowth` and `ocForecast*` already sit behind
`seinterpcompatdispatch::Dispatcher` indirection (newer than the worktree
base). Wrap the Dispatcher call, not a raw legacy call. The E worktree's
diff accounts for this already — reuse its Growth/Forecast case bodies
verbatim.

### Phase F (lines 11391–11404 in current main)

| Opcode | Line | Current | Target |
|---|---|---|---|
| `ocFilter` | 11391 | `ScFilter();` | `if (!tryPlanEngineSpillFilter()) ScFilter();` |
| `ocSort` | 11392 | `ScSort();` | `if (!tryPlanEngineSpillSort()) ScSort();` |
| `ocSortBy` | 11393 | `ScSortBy();` | `if (!tryPlanEngineSpillSortBy()) ScSortBy();` |
| `ocDrop` | 11394 | `ScTakeOrDrop(false);` | `if (!tryPlanEngineSpillTakeOrDrop(false)) ScTakeOrDrop(false);` |
| `ocTake` | 11398 | `ScTakeOrDrop(true);` | `if (!tryPlanEngineSpillTakeOrDrop(true)) ScTakeOrDrop(true);` |
| `ocUnique` | 11404 | `ScUnique();` | `if (!tryPlanEngineSpillUnique()) ScUnique();` |

## 4. Runtime stats plumbing

### Phase F — adds new counter triple

`mnSpillEngine{Attempted,Succeeded,Declined}Count`. Four touchpoints:

- Store struct (line 356, after `mnMatrixEngineDeclinedCount`): 3
  `std::atomic<sal_uInt64>` fields.
- `resetScInterpreterDispatchRuntimeStats` (line 510, after matrix reset
  triple): 3 `.store(0, std::memory_order_relaxed)` calls.
- `getScInterpreterDispatchRuntimeStatsSnapshot` (line 563, after matrix
  load triple): 3 `.load(std::memory_order_relaxed)` calls.
- `ScInterpreterDispatchRuntimeStatsSnapshot` struct in interpre.hxx
  (line 94, after `mnMatrixEngineDeclinedCount`): 3 plain `sal_uInt64`
  fields defaulted to `0`.

All four hunks are present verbatim in worktree `agent-a63d52b5` commit
`cc2dea0ce` — apply by hand; they don't conflict with current main.

### Phase E — no new counter triple

E admission helpers reuse existing `mnMatrixEngine*`. Rationale: LINEST /
LOGEST / TREND / GROWTH / FORECAST / FOURIER all produce matrix or
scalar numeric results that live on the same "matrix engine" semantic
surface as MDETERM / TRANSPOSE / MMULT / MINVERSE. Do not add
`mnRegressionEngine*`.

## 5. Test fixtures

### `testSharedInterpreterLinestEngineDispatch` (Phase E)

- Insert at line 1703, after `testSharedInterpreterMatrixEngineDispatch`
  (ends 1702) and before `testSharedInterpreterBadLiteralDispatch`
  (starts 1704).
- Lift body verbatim from `worktree-agent-a549186e` commit `dbeea31c4`
  diff of `ucalc_formula2.cxx` (lines 1365–1448 in that worktree).
- Tab name `"Linest"`, rows 0, 3, 6, 10, 12 on cols 0/1 and col 2 row 12
  for FORECAST scalar. No conflict.

### `testSharedInterpreterSpillEngineDispatch` (Phase F)

- Insert at line 1769, after `testSharedInterpreterBadLiteralDispatch`
  (ends 1768) and before `testSharedInterpreterRangeDispatch`
  (starts 1770).
- Lift body verbatim from `worktree-agent-a63d52b5` commit `cc2dea0ce`
  diff of `ucalc_formula2.cxx`.
- Tab name `"Spill"`, cols E/F/G for source data, col 0 for results at
  rows 0–22. No conflict.

Both tests assert `mnMatrixEngine*` / `mnSpillEngine*` attempt counts
> 0 and check balance (attempted == succeeded + declined).

## 6. Namespace aliases at top of `interpr4.cxx`

Add one line after existing `namespace serpn = ...` at line 132:

```cpp
namespace sespill = spreadsheetengine::core::rpn::spill;
```

Phase E needs no new alias — `serpn::planLinest` / `planLogest` /
`planTrend` / `planGrowth` / `planForecast` / `planFourier` all live in
existing `serpn` namespace.

## 7. Include additions at top of `interpr4.cxx`

Current state: `interpr4.cxx` line 97 has `#include
<spreadsheetengine/runtime/RpnReference.hxx>`. Phase F's include
(`RpnSpill.hxx`) is not yet present. Add in alphabetical order within
the existing runtime/ include cluster (lines 83–101):

Phase E adds two, after `FinancialRuntime.hxx` (line 87):

```cpp
#include <spreadsheetengine/runtime/ForecastEngine.hxx>
#include <spreadsheetengine/runtime/LinestEngine.hxx>
```

Phase F adds one, after `RpnReference.hxx` (line 99):

```cpp
#include <spreadsheetengine/runtime/RpnSpill.hxx>
```

`ForecastEtsEngine.hxx` is not included — Phase E defers
`ocForecast_Ets*` admissions (ETS statistics path is scaffold-only).

## 8. Ordering / dependencies between E and F

E and F can land in either order — fully independent:

- Different substrate headers (non-overlapping include lines).
- Different namespace aliases (E none; F adds `sespill`).
- Different stats counters (E reuses `mnMatrixEngine*`, F adds
  `mnSpillEngine*` triple).
- Different dispatch cases (E touches 13456–13660; F touches
  11391–11404).
- Different tests (E at 1703; F at 1769).

Only shared touchpoint: helper-insertion region at line 8720. Either
ordering compiles.

Recommended: E first, then F. Rationale:

1. E is higher-risk numerical content (three engines, parity-bound to
   legacy epsilon). Landing first gives clean bisect target.
2. F introduces new `mnSpillEngine*` fields in interpre.hxx
   (SC_DLLPUBLIC API) and the anonymous store, which touches a
   publicly-visible struct layout. Landing it second keeps E isolated
   from ABI churn.
3. F's worktree commit bumps `PROJECT_STATUS.md`
   (`interp4_dispatch_engine_attempt_count`). Landing F last lets the
   rollforward correctly incorporate both E and F admissions.

## 9. Commit decomposition

Four commits, in order. Each ends green against
`run_known_regression_gate.sh` (baseline 12 failures).

### Commit 1: computational: admit LINEST/LOGEST/TREND/GROWTH/FORECAST/FOURIER engine

Touches:

- `sc/source/core/tool/interpr4.cxx` — 2 include lines (line 87 area),
  8 admission helper lambdas (line 8720), 6 dispatch-case changes
  (lines 13456–13660).
- `sc/qa/unit/ucalc_formula2.cxx` —
  `testSharedInterpreterLinestEngineDispatch` inserted at line 1703.

### Commit 2: docs: roll forward Phase E regression/forecast admissions

Touches `spreadsheet_engine/docs/PROJECT_STATUS.md` only. Bumps
`interp4_dispatch_engine_attempt_count` 58 → 65 (adds 7 new admissions).
Phase E narrative block after Batch 4 matrix admissions narrative.

### Commit 3: computational: add Batch 5 RpnSpill admissions (5A simple-shape)

Touches:

- `sc/source/core/tool/interpr4.cxx` — 1 include (after line 99), 1
  namespace alias (after line 132), 3 stats-store fields (after line
  356), 3 reset calls (after line 510), 3 snapshot loads (after line
  563), 6 admission helper lambdas (line 8720, after Phase E), 6
  dispatch-case changes (lines 11391–11404).
- `sc/source/core/inc/interpre.hxx` — 3 snapshot struct fields (after
  line 94).
- `sc/qa/unit/ucalc_formula2.cxx` —
  `testSharedInterpreterSpillEngineDispatch` inserted at line 1769.

### Commit 4: docs: roll forward Phase F Batch 5A spill admissions

Touches `spreadsheet_engine/docs/PROJECT_STATUS.md` only. Bumps
`interp4_dispatch_engine_attempt_count` 65 → 71 (6 new). Phase F /
Batch 5A narrative block. Notes 5B shape-reshaping opcodes remain
deferred.

## 10. Risk items

### Risk 1: Existing tail-engine authoritative tests for GROWTH / FORECAST

Tests `testInterpretTailEngineEvaluatorGrowthAuthoritative` (line 3683),
`testInterpretTailEngineEvaluatorGrowthDefaultOn` (line 3715),
`testInterpretTailEngineEvaluatorForecastAuthoritative` (line 5183) set
authority mode and assert
`maFunctionFallbackCount[GrowthProjection]==0` /
`[StatisticalDistribution]==0`.

Mitigation: InterpretTail seam runs upstream of `Interpret()`. Under
authority mode these formulas never reach RPN dispatch, so
`tryPlanEngineGrowth()` / `tryPlanEngineForecast()` never execute. The
counters increment inside the authoritative InterpretTail path, not in
the Dispatcher fallback body. Admission's svMatrix-only scope fence is
an additional safeguard: range-argument formulas decline anyway.

### Risk 2: SORTBY existing test

`ucalc_formula2.cxx:4814` has `=COM.MICROSOFT.SORTBY(A1:B3;B1:B3;1)`
under tail-engine authority mode. Range-argument, scope fence declines.
No risk.

### Risk 3: `testSharedInterpreterMatrixEngineDispatch` counter drift

E admissions reuse `mnMatrixEngine*`. If the existing matrix test ran
with LINEST/TREND formulas its totals would shift. Inspection: the test
only exercises MUNIT, SEQUENCE, TRANSPOSE, MDETERM, MMULT, MINVERSE. No
drift.

### Risk 4: `ocForecast_LIN` = `ocForecast` fallthrough

Current main collapses both into one case (lines 13463–13467). The
admission helper doesn't distinguish — reads `pCur->GetByte()` for param
count and produces scalar via `planForecast`. Works for both.

### Risk 5: `RpnCoercionResult` payload shape

`LinestPlanResult` / `TrendPlanResult` / `FourierPlanResult` wrap a
`MatrixOperand maMatrix`; the helper extracts via
`aPlan.maValue.maMatrix`. `planForecast` returns scalar double directly
(`success(double)` overload per `ForecastEngine.hxx:137`) — the helper
uses `PushDouble(aPlan.maValue)`. Consistent with worktree patch.

### Risk 6: Known-regressions gate

Baseline 12 failures, none in LINEST/GROWTH/FORECAST/FOURIER/FILTER/SORT
family. New code path gated by svMatrix scope fence; declines fall
through to legacy unchanged. If failures drift: tighten the fence; if a
known failure clears, update `CALC_TEST_KNOWN_REGRESSIONS.md` per the
"old regression unintentionally fixed" protocol.

### Risk 7: Helper insertion order collision between E and F

Both want line 8720. Commits serialized. When F is applied, E's helpers
already occupy ~500 lines starting at 8720; F's helpers go right after
E's block. No conflict.

## Source-of-truth files

- `/home/ubuntu/repos/libreoffice/.claude/worktrees/agent-a549186e/`
  (Phase E source, commit `dbeea31c4`)
- `/home/ubuntu/repos/libreoffice/.claude/worktrees/agent-a63d52b5/`
  (Phase F source, commit `cc2dea0ce`)
