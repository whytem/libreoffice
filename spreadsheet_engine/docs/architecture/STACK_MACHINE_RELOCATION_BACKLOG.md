# Stack Machine Relocation Backlog

Status: active relocation backlog

## Purpose

Carry the now-closed authority-transfer pivot forward as one
dependency-ordered relocation backlog that can be worked commit-by-commit.

This backlog starts from the current tree state:

- Phase 4 is now complete: external-reference execution and
  broadcast/jump-matrix matrix-frame semantics have both landed at the upper
  seam through Slices 1-3
- Phase 6 is now complete: duplicate lower-seam `tryPlanEngine*` overlap has
  been retired, and the remaining `tryPlanEngine*` admissions are the
  engine-backed residual classic-entry set rather than duplicate ownership

The intended end state is one production execution story:

`ScFormulaCell::InterpretTail()` -> `tryEvaluateFormula()` ->
`FormulaEvaluator` -> `RpnEvaluator`

`ScInterpreter::Interpret()` remains only as a residual legacy fallback for
families that are explicitly not yet relocated.

## Close-Out Rules

1. Finish missing substrate before deleting overlap.
2. Retire lower-seam `tryPlanEngine*` admissions only after the upper seam can
   carry the same class of work authoritatively.
3. Count lower-seam audit coverage as useful validation, but not as ambient
   authority transfer.
4. Keep docs honest: Phase 4 is closed only because the external-reference and
   matrix-frame gaps are now covered; Phase 6 closes only when overlapping
   lower-seam admissions are gone.

## Execution Tracking

1. Slice 1: complete
   Verified with `CppunitTest_sc_ucalc_formula2` covering
   `testInterpretTailEngineEvaluatorExternalReferenceRoutes` and
   `testExternalRefFunctions`.
2. Slice 2: complete
   Verified with `CppunitTest_sc_ucalc_formula2` covering
   `testInterpretTailEngineEvaluatorAuthoritativeWithFallback`,
   `testInterpretTailEngineEvaluatorInformationPredicateDefaultOn`,
   `testInterpretTailEngineEvaluatorExternalReferenceRoutes`,
   `testExternalRefFunctions`, and
   `testSharedInterpreterReferenceOffsetDispatch`.
3. Slice 3: complete
   Verified with `CppunitTest_sc_ucalc_formula2` covering
   `testInterpretTailEngineEvaluatorMatrixMathDefaultOn`,
   `testInterpretTailEngineEvaluatorExternalReferenceRoutes`,
   `testInterpretTailEngineEvaluatorSingleCellMatrixExactDefaultOn`,
   `testInterpretTailEngineEvaluatorSingleCellMatrixOffsetExactDefaultOn`,
   `testInterpretTailEngineEvaluatorArrayContextIfDefaultOn`,
   `testInterpretTailEngineEvaluatorBroadcastMatrixDefaultOn`,
   `testInterpretTailEngineEvaluatorMultiCellSelectorMatrixDefaultOn`,
   `testInterpretTailEngineEvaluatorMultiCellSpillMatrixDefaultOn`,
   `testInterpretTailEngineEvaluatorMultiCellConditionalMatrixDefaultOn`, and
   `testInterpretTailEngineEvaluatorMixedLocalExternalConditionalMatrixDefaultOn`.
4. Slice 4: complete
   Verified with `CppunitTest_sc_ucalc_formula2` covering
   `testSharedInterpreterReferenceIndexDispatch` and
   `testSharedInterpreterRetiredMatrixReferenceWave`, plus
   `CppunitTest_sc_interpret_tail_corpus` covering
   `testSeamReconciliationTryPushWrapperFloor`.
5. Slice 5: complete
   Verified with `CppunitTest_sc_ucalc_formula2` covering
   `testInterpretTailEngineEvaluatorCriteriaAggregateAuthoritative`,
   `testInterpretTailEngineEvaluatorCriteriaAggregateDefaultOn`,
   `testInterpretTailEngineEvaluatorDatabaseAggregateAuthoritative`,
   `testInterpretTailEngineEvaluatorDatabaseAggregateDefaultOn`,
   `testSharedInterpreterDatabaseDispatch`,
   `testSharedInterpreterDatabaseGetDispatch`,
   `testSharedInterpreterDatabaseVarianceDispatch`, and
   `testSharedInterpreterDatabaseCountDispatch`, plus
   `spreadsheetengine_execution_tests`.
6. Slice 6: complete
   Verified with `CppunitTest_sc_ucalc_formula2` covering
   `testInterpretTailEngineEvaluatorGrowthDefaultOn`,
   `testInterpretTailEngineEvaluatorRegressionStatsDefaultOn`,
   `testInterpretTailEngineEvaluatorRegressionMatrixDefaultOn`,
   `testInterpretTailEngineEvaluatorForecastAuthoritative`, and
   `testInterpretTailEngineEvaluatorStatisticalDistributionDefaultOn`, plus
   `CppunitTest_sc_ucalc_shared_cases` covering
   `testInterpretTailEngineEvaluatorGrowthHelper`,
   `testInterpretTailEngineEvaluatorRegressionMatrixHelper`, and
   `testInterpretTailEngineEvaluatorForecastHelper`, plus
   `spreadsheetengine_execution_tests`.
7. Slice 7: complete
   Verified with `CppunitTest_sc_ucalc_formula2` covering
   `testSharedInterpreterCriteriaCountIfDispatch`,
   `testSharedInterpreterDatabaseDispatch`,
   `testSharedInterpreterDatabaseGetDispatch`,
   `testSharedInterpreterCountEmptyCellsDispatch`,
   `testSharedInterpreterDatabaseVarianceDispatch`,
   `testSharedInterpreterDatabaseCountDispatch`,
   `testSharedInterpreterReferenceAddressDispatch`,
   `testSharedInterpreterLinestEngineDispatch`,
   `testSharedInterpreterControlFlowIfDispatch`, and
   `testSharedInterpreterSpillEngineDispatch`, plus
   `CppunitTest_sc_interpret_tail_corpus` covering
   `testSeamReconciliationTryPushWrapperFloor`,
   `testLowerSeamEngineAttemptsCarryPivotRationale`, and
   `testProjectStatusOwnsCanonicalDashboardMetrics`.
8. Slice 8: complete
   Verified with `CppunitTest_sc_interpret_tail_corpus` covering
   `testProjectStatusOwnsCanonicalDashboardMetrics` and
   `testSeamReconciliationTryPushWrapperFloor`.

## Backlog

### Slice 1: External-reference matrix materialization bridge

Status: complete

Goal: remove the first remaining Phase 4 blocker by letting matrix-consuming
engine admissions consume external single/double refs instead of declining to
legacy.

Scope:

- widen the lower-seam range-to-matrix bridge to accept
  `svExternalSingleRef` / `svExternalDoubleRef`
- reuse the existing external-reference cache helpers instead of adding a new
  Calc-local materialization path
- cover `TRANSPOSE`, `MDETERM`, `MMULT`, and `MINVERSE` with focused external
  reference tests

Definition of done:

- external matrix formulas no longer decline purely because the source is
  external
- targeted matrix dispatch tests pass in forced-core mode

### Slice 2: Upper-seam external-reference completion

Status: complete

Goal: finish the Phase 4 "external refs" item at the authoritative seam rather
than only in lower-seam audit lanes.

Current landing on the tree:

- direct external single-cell refs now materialize authoritatively for
  scalar-root formulas and the first matrix-math consumer
- broader authoritative external-range / external-name support still remains
  open

Scope:

- unify remaining external-ref scalar/range resolution in the compat AST walker
- ensure matrix and reference consumers share one external-ref materialization
  story
- extend authoritative / default-on tests for external refs

Definition of done:

- external refs used by promoted families succeed upstream under
  `InterpretTail`
- no promoted external-ref family requires lower-seam-only special handling

### Slice 3: Matrix-frame parity

Status: complete

Goal: finish the remaining Phase 4 matrix-frame work that still forces Calc to
own broadcast, jump-matrix, or array-context behavior.

Current landing on the tree:

- `InterpretTail` now preserves and projects exact-dimension matrix results
  for matrix-origin formulas instead of collapsing every admitted matrix-origin
  path back to a scalar top-left value
- focused helper coverage now proves that `tryEvaluateFormula()` can preserve
  selector and spill-family matrices when the caller explicitly requests
  matrix-origin semantics, while the default helper path still returns the
  historical top-left scalar
- focused default-on coverage now proves multi-cell `CHOOSECOLS(...)`,
  `HSTACK(...)`, and `IF(range;...;...)` matrix formulas can stay
  authoritative at the upper seam
- matrix `EXACT(...)` now materializes elementwise upstream, which clears the
  1x1 matrix-origin `SUM(IF(EXACT(range);range;0))` blocker and lets that
  classic jump-matrix-style scalar-result shape stay authoritative
- forced-core audit coverage now proves that lower-seam engine admissions own
  classic jump-matrix `IF(...)` reference-branch shapes, including the
  `SUM(IF(EXACT(OFFSET(...):OFFSET(...));OFFSET(...):OFFSET(...);0))` form
- forced-core audit coverage also now proves array-context `OFFSET(...)`
  reference execution in matrix formulas
- the remaining blocker is therefore not those specific lower-seam shapes, but
  ambient authoritative ownership plus the still-open broadcast / generic
  jump-matrix close-out

Scope:

- broadcast-compatible matrix shapes
- jump-matrix semantics
- array-context behavior
- one canonical range-to-matrix materialization story for local and external
  ranges

Definition of done:

- promoted matrix families no longer decline because of broadcast/jump-matrix
  shape limits
- the plan can mark Phase 4 complete

### Slice 4: Seam reconciliation, matrix/reference wave

Status: complete

Goal: start closing Phase 6 by deleting overlapping lower-seam admissions for
the families covered by Slices 1-3.

Current landing on the tree:

- duplicate lower-seam `tryPlanEngine*` overlap is now retired for the
  matrix/reference wave: `COLUMNS`, `ROWS`, `SHEETS`, `COLUMN`, `ROW`,
  `SHEET`, `AREAS`, `OFFSET`, `INDEX`, `MUNIT`, `MDETERM`, `MINVERSE`,
  `MMULT`, `SEQUENCE`, `TRANSPOSE`, and `SORTBY`
- the seam inventory floor therefore drops from 61 to 45 active
  `tryPlanEngine*` attempt sites while keeping the residual scope honest

Scope:

- remove `tryPlanEngine*` admissions whose only remaining job is duplicate
  matrix/reference execution
- update tests and counters so the upper seam is the named owner for those
  classes

Definition of done:

- matrix/reference overlap in `Interpret()` trends materially downward
- Phase 6 open scope narrows to control-flow, spill, criteria/database,
  regression/forecast, plus the host-sensitive `ADDRESS` / `INDIRECT`
  reference helpers

### Slice 5: Criteria/database tail substrate

Status: complete

Current landing on the tree:

- the upper seam and standalone `FormulaEvaluator` now share
  `runtime/RpnDatabase.hxx` for database-query materialization, field-selector
  normalization, OR-row criteria matching, and DB aggregation execution
- COUNTIF/SUMIF/AVERAGEIF-style tails and the DB family (`DSUM`, `DCOUNT`,
  `DCOUNTA`, `DAVERAGE`, `DGET`, `DMAX`, `DMIN`, `DPRODUCT`, `DSTDEV(P)`,
  `DVAR(P)`) now consume that shared engine-native substrate instead of
  bespoke Calc-local loops
- focused authoritative/default-on tests now prove named-range, missing-field,
  and count-without-field DB shapes at the upper seam, while the existing
  shared-interpreter audit tests still cover the lower seam

Goal: finish the remaining host-sensitive criteria/database substrate in a form
that can later retire the corresponding lower-seam admissions.

Scope:

- remaining COUNTIF/SUMIF/AVERAGEIF-style tails
- database-family materialization and criteria matching parity

Definition of done:

- criteria/database families consume shared engine-native substrate rather than
  bespoke Calc-local loops

### Slice 6: Regression / forecast substrate

Status: complete

Current landing on the tree:

- the upper seam now classifies scalar regression statistics
  (`SLOPE`, `CORREL`, `PEARSON`, `RSQ`, `STEYX`, `COVAR`,
  `COVARIANCE.P`, `COVARIANCE.S`) as `StatisticalDistribution`, so the
  existing engine-native regression stats path stays authoritative under
  `InterpretTail`
- `LINEST`, `LOGEST`, `TREND`, and `GROWTH` now materialize matrix results
  upstream through the shared `LinestEngine` substrate instead of relying on
  interpreter-resident kernels
- scalar helper paths now read the top-left value from that same shared
  matrix result, and nested consumers like `INDEX(LOGEST(...))` can consume
  those matrices authoritatively
- the remaining regression/forecast work is therefore no longer math-kernel
  ownership; it is the Slice 7 deletion pass over the overlapping
  lower-seam admissions

Goal: move the remaining numerical cores that still block end-to-end stack
machine relocation.

Scope:

- forecast / regression engines
- parity and numerical-stability tests

Definition of done:

- forecast/regression no longer need interpreter-resident execution kernels

### Slice 7: Seam reconciliation, residual wave

Status: complete

Current landing on the tree:

- duplicate lower-seam `tryPlanEngine*` overlap is now retired for the
  criteria/database wave, the regression/forecast wave, and the remaining
  promoted legacy-fallback helpers `ADDRESS`, `INDIRECT`, and
  `ORG.LIBREOFFICE.FOURIER`
- `interp4_dispatch_plan_engine_attempt_count` now measures only duplicate
  overlap, and that count is `0`
- the remaining `21` lower-seam `tryPlanEngine*` sites are now tracked
  separately as engine-backed residual classic entry points for
  `IF` / `IFERROR` / `IFNA` / `CHOOSE` / `IFS` / `SWITCH` and the spill
  family, not as evidence of two owners for the same promoted class
- Phase 6 can therefore be marked complete; Slice 8 is now a documentation
  and handoff close-out rather than another authority-transfer code wave

Goal: finish Phase 6 by deleting the remaining overlapping lower-seam
admissions once Slices 5-6 land.

Scope:

- criteria/database lower-seam admissions
- regression/forecast lower-seam admissions
- headline dashboard cleanup so lower-seam attempt counts stop standing in for
  progress on promoted classes

Definition of done:

- the pivot plan can mark Phase 6 complete
- the project can name a single owner for every promoted family

### Slice 8: Plan closure and relocation handoff

Status: complete

Current landing on the tree:

- the pivot plan now records authority transfer as complete on the current
  tree and hands active work to the relocation backlog
- `PROJECT_STATUS.md` now treats live evaluation authority transfer as
  complete and documents the remaining Calc-owned legacy surface in plain
  relocation terms
- the residual classic surface is now named explicitly as:
  - the `ocBad` / `ocRange` parity-gap wrappers plus the 9 retired-scalar
    audit sites
  - the `21` engine-backed spill/control-flow classic-entry sites
  - the remaining interpreter-resident legacy lambdas, legacy dispatch
    targets, and host/application-state utilities measured on the dashboard
- active work is therefore no longer framed as authority transfer; it is the
  ordinary relocation queue below

Goal: close the pivot plan honestly and turn the remaining work into plain
module relocation rather than authority-transfer bookkeeping.

Scope:

- update the pivot plan and project status from `partial` to `complete`
- document the residual legacy surface that still belongs in Calc
- define the post-pivot relocation queue for any interpreter-state utilities
  still worth moving into `spreadsheet_engine`

Definition of done:

- the pivot plan is fully closed out
- remaining work is no longer framed as authority transfer

## Working Order

1. Slice 1
2. Slice 2
3. Slice 3
4. Slice 4
5. Slice 5
6. Slice 6
7. Slice 7
8. Slice 8

This order is intentional: it finishes the missing substrate first, then uses
that substrate to delete overlap, then closes docs only after the code path is
real.

## Post-Pivot Relocation Queue

1. Reduce `interp4_dispatch_legacy_lambda_count` by moving reusable
   interpreter-resident helpers into `spreadsheet_engine` or deleting them
   outright once parity is proven elsewhere.
2. Reduce `legacy_interpreter_subroutine_count` by relocating the remaining
   engine-worthy interpreter subroutines behind explicit host/module
   contracts.
3. Shrink `interp4_dispatch_engine_backed_plan_engine_attempt_count` by
   deciding which spill/control-flow classic-entry sites should move upstream
   and which should remain Calc-owned legacy surface.
4. Retire the two older `tryPushEngine*` wrappers (`ocBad` and `ocRange`) once
   their parity-gap reference/error handling is covered elsewhere.
5. Extract interpreter-state utilities that are still generally useful to the
   engine module, while leaving document/application host policy in Calc.
