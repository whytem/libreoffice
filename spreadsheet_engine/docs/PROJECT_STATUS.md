# Spreadsheet Engine: Project Status

This file is the concise current-state snapshot for `spreadsheet_engine/`.

Start here for the active migration story:

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md](architecture/COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md](architecture/COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_RPN_BATCH_EXECUTION_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_RPN_BATCH_EXECUTION_PLAN.md)
- [architecture/CALC_TEST_KNOWN_REGRESSIONS.md](architecture/CALC_TEST_KNOWN_REGRESSIONS.md)

## Objective

The long-term goal remains to make `spreadsheet_engine/` the home for Calc's
spreadsheet computation engine while Calc remains the document and
application host.

That splits into two tracks:

- shared-engine extraction: materially achieved
- live evaluation authority transfer inside Calc: in progress

## Live Evaluator Dashboard

### Replay Guardrail

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

### North-Star Live Authoritative Match

This is the deletion-gating number for the standing replay corpus:

- `interpret_tail_live_authoritative_corpus_formula_cells=50661`
- `interpret_tail_live_authoritative_probe_formula_cells=50386`
- `interpret_tail_live_authoritative_match_total=50382`
- `interpret_tail_live_authoritative_fallback_total=4`
- `known_regressions_baseline=0`
- `legacy_interpreter_subroutine_count=61`
- `interp4_dispatch_legacy_lambda_count=25`
- `interp4_dispatch_legacy_dispatch_target_count=62`
- `interp4_dispatch_legacy_call_count=80`
- `interp4_dispatch_engine_attempt_count=81`
- `interp4_dispatch_engine_attempted_total=0`
- `interp4_dispatch_engine_succeeded_total=0`
- `interp4_dispatch_engine_declined_total=0`
- `interp4_dispatch_engine_attempted_total_core_forced_full_legacy=602`
- `interp4_dispatch_engine_succeeded_total_core_forced_full_legacy=602`
- `interp4_dispatch_engine_declined_total_core_forced_full_legacy=0`
- `interp4_dispatch_controlflow_engine_attempted_total=0`
- `interp4_dispatch_controlflow_engine_succeeded_total=0`
- `interp4_dispatch_controlflow_engine_declined_total=0`
- `interp4_dispatch_reference_engine_attempted_total=0`
- `interp4_dispatch_reference_engine_succeeded_total=0`
- `interp4_dispatch_reference_engine_declined_total=0`
- `sc_formula_executor_formula_cell_interpret_total_live=2446901`
- `sc_formula_executor_formula_group_attempt_total_live=1220544`
- `sc_formula_executor_formula_group_handled_total_live=105`
- `sc_formula_executor_interpret_tail_total_live=2442428`
- `sc_formula_executor_classic_interpret_total_live=0`
- `sc_formula_executor_formula_cell_interpret_total_core_forced_full_legacy=105004`
- `sc_formula_executor_formula_group_attempt_total_core_forced_full_legacy=75516`
- `sc_formula_executor_formula_group_handled_total_core_forced_full_legacy=0`
- `sc_formula_executor_interpret_tail_total_core_forced_full_legacy=90144`
- `sc_formula_executor_classic_interpret_total_core_forced_full_legacy=602`
- `interp4_dispatch_legacy_quarantine_missing_dispatch_target_count=0`
- live authoritative-match rate over the corpus: `99.4493%`
- live authoritative-match rate over the current promoted probe: `99.9921%`

The north-star measures live authority transfer. `legacy_interpreter_subroutine_count`
is the blunt retirement-progress companion metric, derived from the remaining
`void Sc*()` declarations in [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx).
Lower is better. `interp4_dispatch_legacy_lambda_count` is the relocated-legacy
companion metric: it counts `pushLegacy*` lambdas still living inside
[interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx).
If the wrapper count falls while the lambda count stays flat or rises, we are
relocating Calc logic rather than moving authority into the standalone engine.
`interp4_dispatch_engine_attempt_count` is the static companion for the first
real engine-first dispatch work inside `Interpret()`: it counts dispatch cases
that now try the standalone engine first before falling back to Calc. The
paired runtime totals show whether that path is actually carrying replay load.
On the standing live corpus those totals are still `0 / 0 / 0`, which is still
an honest sign that the upstream seam prevents this path from seeing ordinary
replay traffic. But the core-forced full-legacy audit lane now reports
`602 / 602 / 0`, so the engine-first dispatch path is no longer theoretical:
it is carrying the entire residual classic tail when the classic interpreter is
actually exercised. The `ocRange` audit turned out to be especially useful:
all `96` residual `Range` rows were really bracketed ODF error-literal syntax
like `=[.OF:.ERR]:502`, not true reference-range work. Those rows now route
through the engine-backed bad-literal path, while the focused dynamic-range
lane still proves valid `OFFSET(...):OFFSET(...)` range construction can
succeed through the dedicated range path. The `ocBad` switch case is now the
first real retirement through engine authority: the legacy `ScBadName()` path
is deleted, and the classic interpreter no longer coexists with an alternate
Calc implementation for root error literals. The classic opcode census still
shows `Bad=506` and `Range=96` because it counts opcode entry before the
switch decides whether engine or legacy computes the result. So the next bottleneck is
no longer root error literals or this faux-range tail; it is the remaining real
reference and control/matrix substrate behind the still-unseen live surface.

Batch 1 of the five-batch RPN evaluator plan has now landed its substrate
(`runtime/RpnControlFlow.hxx`) and six explicit admissions:
- `ocIf` with a scalar condition routes through `planIfBranch`.
- `ocChoose` (CHOOSE) routes through `planChooseBranch`.
- `ocIfError` / `ocIfNA` route through `planIfErrorBranch` when the
  primary value at top of stack is a simple scalar (svDouble /
  svString / svError / svEmpty / svMissing) and there is no
  pre-existing global error.
- `ocIfs_MS` (IFS) routes through `planIfsBranch` driven one pair at
  a time when every condition token is a simple scalar; the selected
  pair's value is preserved as a `FormulaConstTokenRef` so any
  result type survives the stack drop.
- `ocSwitch_MS` (SWITCH) routes through `planSwitchBranch` when the
  selector and every case label are simple scalars; case-result and
  default slots are resolved off the un-reversed param window.
Reference, matrix, external-ref, and jump-matrix shapes still defer
to the legacy `pushLegacy*` lambdas, which own the matrix-frame
`JumpMatrix` protocol until Batch 4 lands. `ocLet` remains deferred
pending the nested-interpreter spawn contract. The three
`controlflow_engine_*` runtime totals stay at `0 / 0 / 0` on the
live corpus because the upstream seam captures virtually all
control-flow traffic before reaching `Interpret()`, matching the
expected shape documented in the initiative policy.

Batch 2 substrate (`runtime/RpnReference.hxx`) has now landed together with
five admissions:
- `ocColumn` / `ocRow` / `ocSheet` with no argument or a single-reference
  argument route through `planAxisOrdinal`.
- `ocColumns` / `ocRows` / `ocSheets` with a single `svSingleRef` or
  `svDoubleRef` argument route through `planSpanCount`.
- `ocAreas` with a single scalar reference routes through
  `planAreaCount(1)`.
- `ocOffset` in its 3-argument form with a single-reference base and
  scalar row/col offsets routes through `planOffset`.
- `ocIndex` with scalar row/col indices on a single-reference or
  double-reference base routes through `projectIndexReference`. Only the
  Scalar and KeepSource selection kinds are accepted; RowSlice /
  ColumnSlice (zero-axis matrix returns) still defer to legacy pending
  the Batch 4 matrix-materialization contract.
- `ocAddress` in its narrow 2-argument form (row, col) routes through
  `serefexec::formatAddressFunctionResult` with default A1 style and
  absolute mode 1. Any 3-5 argument form with abs mode, style flag, or
  sheet token defers to legacy until the parameter-parsing contract is
  extended.
Matrix-context no-arg, external-ref, multi-argument, 5-arg OFFSET with
new-height/new-width, and INDEX zero-axis forms defer to legacy. Three new
`reference_engine_*` runtime totals are published above, currently
`0 / 0 / 0` in the live lane for the same seam-captures-upstream reason
as the control-flow counters. `ocIndirect` is now the sixth Batch 2
admission: it routes through `seindirectexec::resolveIndirectReference`
when the reference text is already an svString token and the optional
A1/R1C1 flag is a scalar svDouble; the helper preserves the existing
syntax-policy resolution and pushes the resolved Single / Double /
ExternalSingle / ExternalDouble / Token result. `ocMultiArea` remains
on the trivial `ScUnionFunc` wrapper with no computation to migrate;
INDEX matrix-return form and wider ADDRESS parameter combinations are
gated on the Batch 4 matrix-materialization contract. Batch 2 is
substantively complete and the initiative advances to Batch 3.

Batch 3 substrate (`runtime/RpnCriteria.hxx` + `runtime/RpnDatabase.hxx`)
has landed plus three admissions: `ocCountIf`, `ocSumIf`, and
`ocAverageIf` now try the engine-native
`planSingleCriterionAggregate` before falling back to legacy
`ScCountIf` / `IterateParametersIf(ifSUMIF|ifAVERAGEIF)`. Acceptance
is scope-fenced to svDoubleRef ranges with a single-sheet absolute
resolution and a scalar svDouble or svString criterion; external refs,
RefList, matrices, multi-sheet ranges, and non-scalar criteria defer
to legacy. The existing
`seitee::detail::CriteriaAggregateMaterializer` serves as the host
bridge for cell materialization, so this admission did not require new
Host facade work. Three new `criteria_engine_*` runtime totals are
published and currently `0 / 0 / 0` on the live corpus for the same
seam-captures-upstream reason as the control-flow and reference
counters. The IFS family (`ocCountIfs` / `ocSumIfs` / `ocAverageIfs` /
`ocMinIfs_MS` / `ocMaxIfs_MS`) now also routes through
`planMultiCriterionAggregate` via the shared
`tryPlanEngineMultiCriterionAggregate(kind, withTargetRange)` helper.
Each (criteria_range, criterion) pair is validated through the same
`buildCriteriaRangeInput` bridge as the single-criterion family, and
the optional target range (SUMIFS / AVERAGEIFS / MINIFS / MAXIFS) is
accepted when it is an svDoubleRef on a single sheet.
`ocCountEmptyCells` now routes through `serpn::planCountEmptyRange`,
which streams a `CriteriaAggregateInput` range through the same
materializer the other criteria admissions use and counts cells whose
`CellValue` reports Empty or empty Text — mirroring legacy
`isCellContentEmpty` semantics. Scope fence: single-sheet svDoubleRef
only; svSingleRef / svRefList / svMatrix / external refs defer.

Batch 3 tail admissions now additionally cover DCOUNT / DCOUNTA and
the DB variance family plus DGET. Two new `CriteriaAggregateKind`
values, `Count2` and `CountNumeric`, carry legacy DCOUNTA /
DCOUNT-with-field-specified semantics through
`evaluateCriteriaAggregate`; both plug into the existing
`tryPlanEngineDatabaseAggregate` helper, declining only for the
bMissingField branch of DCOUNT where no field column resolves.

`RpnVariance.hxx` provides a Welford-style `VarianceAccumulator` plus
`planDatabaseVarianceAggregate` for the DSTDEV / DSTDEVP / DVAR /
DVARP family and `planDatabaseGet` for DGET's uniqueness-enforcing
return. Both planners drive the same CriteriaAggregateMaterializer,
stream the criteria grid, and finalize as:
  - variance: population / sample divisor, optional sqrt, errors on
    insufficient count
  - get: exactly one match -> success; zero -> NoValue; two or more
    -> IllegalArgument
The Calc-side helpers `tryPlanEngineDatabaseVariance` and
`tryPlanEngineDatabaseGet` share the 3-arg DB scope fence
(single-sheet svDoubleRef on both ranges, exactly one criteria data
row, scalar field by index or header name). External refs, RefList,
multi-sheet, multi-criteria-row, and missing-field cases still defer
to legacy.

Batch 4 substrate (`runtime/RpnMatrix.hxx`) has landed with six
admissions so far. The pure-scalar constructors `ocMatrixUnit`
(MUNIT) and `ocMatSequence` (SEQUENCE) route through
`serpn::planIdentityMatrix` / `planSequenceMatrix`: they produce
matrices from scalar-only arguments, so no host-side range
materialization is needed. `ocMatTrans` (TRANSPOSE), `ocMatDet`
(MDETERM), `ocMatMult` (MMULT), and `ocMatInv` (MINVERSE) exercise
the matrix-consuming path for in-memory `svMatrix` tokens via
`serpn::planTranspose` / `planDeterminant` / `planMatrixMultiply` /
`planMatrixInverse`. Two bridges span the host boundary:
`convertMatrixOperandToMatrixRef` copies the engine's
`MatrixOperand` (row-major `std::vector<CellValue>`) into an
`ScMatrixRef` for `PushMatrix`, and `convertMatrixRefToMatrixOperand`
does the reverse for svMatrix inputs. Range-input widening for
TRANSPOSE / MDETERM / MMULT / MINVERSE and the SUMPRODUCT /
regression-forecast family still need a reference-to-matrix
materialization primitive on the Host facade and decline to the
legacy path when their argument arrives as `svDoubleRef` /
`svSingleRef` / `svRefList`.

Three new `matrix_engine_*` runtime totals are published. A separate
cleanup fix gated `importedRootUsesStoredHostValueTruth` behind the
imported-root predicate, clearing 12 tests from the known-regressions
list (30 -> 18): the fallback was firing in live-recalc contexts
where the "host cell value" was the previous result of the same
formula, leaving `=SHEETS()` / `=IF(...)` / `=FTEST(...)` etc. frozen
at zero after the first recalc.

The DB aggregate family first admission: `ocDBSum` /
`ocDBAverage` / `ocDBMax` / `ocDBMin` route through
`tryPlanEngineDatabaseAggregate(kind)` which translates the
3-argument database-query shape into a `planMultiCriterionAggregate`
call. The helper walks the criteria header, matches each non-empty
criteria column to a database column by header-text equality, and
builds a parallel `(CriteriaAggregateInput, CriteriaPredicate)` pair
per matched column. The existing
`seitee::detail::CriteriaAggregateMaterializer` plus
`readMaterializedHostCellValue` cover the host-side cell reads.
Scope fence: single-sheet svDoubleRef ranges; database ≥ 2 rows;
criteria exactly 2 rows (header + one criteria data row; multi-row OR
criteria defer to legacy); field is svDouble (1-based index) or
svString (matching a database header); criteria cells must be Number
/ Text / Boolean. The `CriteriaAggregateKind` enum has been extended
with a `Product` accumulator (empty-product yields 0 to match legacy
DBProduct), wiring `ocDBProduct` through the same scope-fenced
admission alongside Sum / Average / Max / Min. Remaining DB family
(`ocDBCount` / `ocDBCount2` / `ocDBGet` / `ocDBStdDev(P)` /
`ocDBVar(P)`) still defers — each needs its own iteration path
(count-with-missing-field, count-non-empty-including-text,
unique-match scalar return, variance accumulator).

`ocMatDet` (MDETERM) is the fourth Batch 4 admission: it routes
through `serpn::planDeterminant` when the single argument is an
svMatrix token, reusing the same `convertMatrixRefToMatrixOperand`
bridge as TRANSPOSE.

`ocMatMult` (MMULT) and `ocMatInv` (MINVERSE) are the fifth and
sixth Batch 4 admissions (Phase C of the Close-Out Plan). Both
route through engine-native numerical cores:
`semath::evaluateMatrixMultiply` walks the same ascending-k Kahan
summation order as legacy `ScInterpreter::ScMatMult`, and
`semath::evaluateMatrixInverse` runs an LUP decomposition whose
singular-matrix policy matches legacy `ScMatInv` exactly (zero
max-absolute row during scaling, or zero on the diagonal after
decomposition, both surface as `Error::IllegalArgument` → `#VALUE!`).
No separate epsilon is applied; the legacy code's strict
equality-to-zero test is preserved so near-singular-but-non-zero
pivots still invert. `serpn::planMatrixMultiply` and
`serpn::planMatrixInverse` bridge the numerical cores to the RPN
matrix operand shape via a shared `tryFlattenNumericMatrix` helper.
Scope fence: MMULT / MINVERSE require in-memory svMatrix tokens;
range-input widening for these two waits on the same host-facade
primitive used below by TRANSPOSE / MDETERM.

Phase D of the Close-Out Plan widens the TRANSPOSE and MDETERM
admissions to accept svSingleRef / svDoubleRef range tokens by
landing the first documented Host facade primitive,
`seitee::detail::materializeHostRangeToMatrixOperand`. The helper
wraps the existing `readMaterializedHostCellValue` pipeline and
produces a `serpn::MatrixOperand` directly, so the engine-first
dispatch path no longer declines when a single-cell or range
reference reaches the TRANSPOSE / MDETERM admissions (e.g.
`=MDETERM(A1)`, `=TRANSPOSE(A1:C2)`). svRefList and multi-sheet
surfaces still decline; MMULT / MINVERSE / the SUMPRODUCT family
will pick up the same bridge when their own admissions extend.
The contract for this primitive is tracked in
[architecture/HOST_FACADE_CONTRACTS.md](architecture/HOST_FACADE_CONTRACTS.md);
Phase I will extend that document with the remaining address /
range-resolution / iteration / spill contracts.

Batch 4 regression/forecast admissions (Phase E of the Close-Out
Plan) route seven additional opcodes through the engine-first
dispatch. `ocLinest` / `ocLogest` (LINEST / LOGEST) route through
`serpn::planLinest` / `planLogest`, `ocTrend` / `ocGrowth` (TREND /
GROWTH) through `serpn::planTrend` / `planGrowth`, `ocForecast_LIN`
/ `ocForecast` (FORECAST) through `serpn::planForecast`, and
`ocFourier` (FOURIER) through `serpn::planFourier`. GROWTH and
FORECAST additionally keep the existing
`Dispatcher::growth` / `Dispatcher::forecast` indirection and its
`warnIfLegacyGrowthProjectionReached` /
`warnIfLegacyStatisticalDistributionReached` observability on the
decline path, so the legacy audit surface is preserved. Scope
fence: matrix inputs must arrive as svMatrix tokens, flags must be
svDouble; range tokens decline to the legacy path pending the same
reference-to-matrix widening that TRANSPOSE / MDETERM already use.
The admissions reuse the `mnMatrixEngine*` counters because the
regression/forecast family produces matrix or scalar numeric
results on the same semantic surface as MDETERM / TRANSPOSE / MMULT
/ MINVERSE. No new counter triple is introduced.

`ocLet` remains the last unstarted Batch 1 member; the nested-
interpreter spawn contract for binding resolution is the gating
substrate work.

Batch 5A substrate (`runtime/RpnSpill.hxx`) has landed with six
admissions covering the simple-shape dynamic-array family:
`ocFilter`, `ocSort`, `ocSortBy`, `ocUnique`, `ocTake`, `ocDrop`
now try `sespill::planFilter` / `planSort` / `planUnique` /
`planTake` / `planDrop` before falling back to legacy
`ScFilter` / `ScSort` / `ScSortBy` / `ScUnique` / `ScTakeOrDrop`.
The planners are pure functions over the engine's
`MatrixOperand` substrate; geometry delegates to
`spreadsheetengine::api::array::planTakeDropSlice` for the
TAKE/DROP windows. Scope fence: every source must arrive as an
svMatrix token, every option argument must be a scalar svDouble
or svMissing. Range inputs defer to legacy pending the Phase D
reference-to-matrix materialization contract. The spill
admission accounting publishes three new `spill_engine_*`
runtime totals on the dispatch stats snapshot. The Host facade
adds a dedicated `SpillRangeAllocator` contract plus a
header-only libreoffice compat adapter
(`compat/libreoffice/SpillAllocation.hxx`) that wraps
ScDocument for collision probing and rectangle allocation.
Phase 5A admissions do NOT reach the allocator yet — they
PushMatrix directly through `convertMatrixOperandToMatrixRef`,
matching the legacy behavior; the allocator lifecycle lands
with Phase 5B when the shape-reshaping family needs it, and it
is documented up-front that `#SPILL!` on collision is NEW
behavior not a migration of existing behavior.

Phase 5B has now landed ten shape-reshaping admissions covering
the full dynamic-array family: `ocHStack`, `ocVStack`,
`ocChooseCols`, `ocChooseRows`, `ocExpand`, `ocToCol`, `ocToRow`,
`ocWrapCols`, `ocWrapRows`, and `ocTextSplit` each try a
`sespill::plan*` planner before falling back to the legacy
`ScHorizontalOrVerticalStack` / `ScChooseColsOrRows` / `ScExpand`
/ `ScToColOrRow` / `ScWrapColsOrRows` / `ScTextSplit` body.  The
new planners (`planHStackOrVStack`, `planChooseColsOrRows`,
`planExpand`, `planToColOrRow`, `planWrapColsOrRows`,
`planTextSplit`) are pure functions over `MatrixOperand`;
geometry delegates to the existing `searray` helpers
(`appendStackDimensions`, `planChooseResultDimensions`,
`planExpandDimensions`, `planFlattenOutputDimensions`,
`planWrapOutputDimensions`, `wrapDestination`).  Scope fence
mirrors Phase 5A: every matrix argument must arrive as an svMatrix
token; numeric options must be scalar svDouble / svMissing; string
options for EXPAND / WRAPCOLS / WRAPROWS / TEXTSPLIT pad values are
scalar svString.  Range inputs decline and defer to legacy pending
the Phase D reference-to-matrix materialization contract.
TEXTSPLIT's delimiter matching uses an engine-local ASCII
case-folding fallback when `bMatchMode` is set; the legacy
ScGlobal CharClass Unicode fold remains accessible through the
legacy decline path for workbooks that need full Unicode semantics.
`interp4_dispatch_engine_attempt_count` moves from `71` to `81`
with the ten admissions.

The current value reflects the restored original `Sc*` names after backing out
earlier rename-only metric compression, and the quarantine audit currently
shows `62 / 62` dispatch-reachable legacy lambdas warning when reached. This
latest drop came from moving the compatibility-heavy statistical / aggregate /
test / growth block out of `Interpret()` and into
[InterpreterCompatDispatch.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpreterCompatDispatch.hxx),
so it is a real reduction in the relocated legacy surface rather than another
wrapper-count-only cleanup.

## Next Initiative: Engine RPN Evaluator

The remaining migration work is now better understood as one subsystem
initiative than as another flat queue of leaf functions.

What remains is dominated by:

- operator semantics over polymorphic stack values
- jump/control-flow opcodes
- reference-shaped operands
- matrix-frame state
- criteria/database iteration
- stack/runtime state such as error and format propagation

That go-forward path is now tracked in:

- [architecture/COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md](architecture/COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md](architecture/COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md)

Immediate consequence:

- the next honest migration metric is reduction in
  `interp4_dispatch_legacy_lambda_count`
- `interp4_dispatch_engine_attempt_count` is now a companion, not a success
  metric by itself; it must be read alongside the live and full-legacy
  runtime totals
- no new `pushLegacy*` lambdas should be treated as progress unless they are
  temporary compatibility fallbacks for already engine-owned roots
- the first prerequisite before opcode-by-opcode migration is a fixed
  host-boundary audit

Everything below is diagnostic context for improving that number.

### Full Replay Corpus: Ambient Live Observe Attempts

These are attempt totals from live observe, not unique-cell coverage. They
remain useful for hotspot steering, but they should not be read as the
deletion denominator.

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=1956550`
- `interpret_tail_live_fallback_total=0`
- `interpret_tail_live_seen_total=1956550`
- `interpret_tail_live_unseen_formula_cells=0`
- `interpret_tail_live_promoted_function_supported_total=1956434`
- `interpret_tail_live_supported_rate=3862.04`
- `interpret_tail_live_seen_rate=3862.04`

Ambient live fallback reasons:

- `unsupported_formula_shape=0`
- `unsupported_host_surface=0`
- `parse_failure=0`
- `unsupported_function=0`

### Full Replay Corpus: Live Unique-Cell Surface

This is the honest per-formula-cell live-routing surface from the standing
replay corpus. It counts whether each formula cell was actually seen and
supported during the bulk live observe run.

- `interpret_tail_live_unique_formula_cells=50661`
- `interpret_tail_live_unique_seen_formula_cells=50640`
- `interpret_tail_live_unique_supported_formula_cells=50640`
- `interpret_tail_live_unique_fallback_formula_cells=0`
- `interpret_tail_live_unique_unsupported_function_formula_cells=0`
- `interpret_tail_live_unique_unseen_formula_cells=21`
- `interpret_tail_live_unique_seen_rate=99.90`
- `interpret_tail_live_unique_supported_rate=99.90`

### Live Unique Unsupported-Function Top-N

This table is now intentionally empty on the live unique surface. It remains
the right place to publish any new regression if one appears.

- `interpret_tail_live_unique_unsupported_function_formula_cells=0`

Next routing policy:

- keep `unsupported_function=0` as an explicit regression guard
- steer the next phase off unseen live surface and genuine reduction in
  `interp4_dispatch_legacy_lambda_count`
- treat growth in `interp4_dispatch_engine_attempt_count` without corresponding
  movement in the runtime totals as a new gaming risk to guard against
- treat wrapper deletion as secondary unless the relocated legacy dispatch
  surface also falls
- use the host-boundary audit as the design gate for the next subsystem work

### Unknown Bucket Root Split

There is no remaining live unique `FunctionKind::Unknown` unsupported-function
residue on the validated standing corpus, but there is still a small unseen
unknown-surface tail.

Top unseen unknown-surface roots:

- `parse_failure`: `14` formula cells, `12` unseen
- `root:array_constant`: `10` formula cells, `0` unseen
- `COM.MICROSOFT.COVARIANCE.P`: `7` formula cells, `0` unseen

The unknown-root samples made the last residual `operator:+` band legible:
those `34` unseen cells were imported `TODAY() + n` date-offset formulas from
`sequence.fods`, not a generic scalar-operator gap. The engine now admits
`TODAY()` as a scalar child inside promoted scalar-root expressions, which is
why that bucket disappears from the live unknown-surface table without widening
root `TODAY()` into a new global default-on family.

Updated next routing policy:

- the next big value is retirement plus unseen-surface reduction inside the
  remaining unknown-surface tail, not more unsupported-function widening
- the current live routing surface is broad enough that the deletion metric
  and the north-star should lead decision-making

### Full Replay Corpus: Forced Interpret Observe Attempts

These are also attempt totals. The new unique-cell direct surface is the
honest coverage metric below.

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=2660369`
- `interpret_tail_forced_interpret_fallback_total=0`
- `interpret_tail_forced_interpret_seen_total=2660369`
- `interpret_tail_forced_interpret_unseen_formula_cells=0`
- `interpret_tail_forced_interpret_promoted_function_supported_total=2660304`
- `interpret_tail_forced_interpret_supported_rate=5251.32`
- `interpret_tail_forced_interpret_seen_rate=5251.32`

### Full Replay Corpus: Forced Direct Unique-Cell Surface

This is the direct-routing comparison surface after dirtying and forcing each
replay formula cell once, then classifying whether that formula cell was
actually seen and supported by the seam.

- `interpret_tail_forced_direct_formula_cells=50661`
- `interpret_tail_forced_direct_seen_formula_cells=44773`
- `interpret_tail_forced_direct_supported_formula_cells=44773`
- `interpret_tail_forced_direct_fallback_formula_cells=0`
- `interpret_tail_forced_direct_unseen_formula_cells=5888`
- `interpret_tail_forced_direct_seen_rate=88.38`
- `interpret_tail_forced_direct_supported_rate=88.38`

### Raw Cached-Workbook Promoted Probe

- `interpret_tail_probe_formula_cells=50386`
- `interpret_tail_authoritative_total=300`
- `interpret_tail_authoritative_fallback_total=50086`
- raw promoted authoritative rate: `0.61%`

Raw promoted fallback reasons:

- `shadow_mismatch=50086`
- `unsupported_function=0`
- `unsupported_formula_shape=0`
- `unsupported_host_surface=0`

### Live-Reachable vs Imported-Artifact Promoted Probe Split

- `interpret_tail_probe_live_reachable_formula_cells=300`
- `interpret_tail_live_target_authoritative_total=300`
- `interpret_tail_live_target_authoritative_fallback_total=0`
- `interpret_tail_probe_live_reachable_rate=0.60%`
- `interpret_tail_probe_imported_artifact_formula_cells=50086`
- `interpret_tail_probe_host_truth_artifact_formula_cells=50086`
- `interpret_tail_probe_imported_artifact_rate=99.40%`

Interpretation:

- the raw promoted probe is now overwhelmingly imported cached-workbook debt,
  not a live parity denominator
- the promoted probe is only interpretable when split into live-reachable vs
  imported-artifact-only buckets
- the `shadow_mismatch=50086` wall is real diagnostic debt, but it is almost
  entirely on imported-artifact-only rows rather than live-reachable parity rows

### Promoted Replay Eligibility Inventory

- `interpret_tail_replay_promoted_formula_cells=50386`
- `interpret_tail_replay_promoted_direct_seen=44743`
- `interpret_tail_replay_promoted_direct_supported=44743`
- `interpret_tail_replay_promoted_direct_fallback=0`
- `interpret_tail_replay_promoted_direct_unseen=5643`
- `interpret_tail_replay_promoted_shared_formula_cells=40577`
- `interpret_tail_replay_promoted_non_shared_formula_cells=9809`
- `interpret_tail_replay_promoted_unseen_shared_member=2636`
- `interpret_tail_replay_promoted_unseen_non_shared=2743`
- `interpret_tail_replay_promoted_shared_member_seen_via_top=0`
- `interpret_tail_replay_promoted_needs_interpret_after_dirty=50386`
- `interpret_tail_replay_promoted_dirty_after_interpret=5643`

### Engine-Authoritative Families

Default-on families:

- logical constants: `TRUE()` / `FALSE()`
- scalar-root formulas: comparisons, arithmetic roots, unary roots, and
  percent roots
- formula text: `FORMULA(...)`
- conversion family: `CONVERT(...)`, `ORG.OPENOFFICE.CONVERT(...)`,
  `EUROCONVERT(...)`, `BASE(...)`, `DECIMAL(...)`, `ROMAN(...)`,
  `ARABIC(...)`
- round family: `ROUND(...)`, `ROUNDUP(...)`, `ROUNDDOWN(...)`
- significant rounding: `ROUNDSIG(...)`, `ORG.LIBREOFFICE.ROUNDSIG(...)`
- math-scalar family: bounded scalar trig, inverse-trig, hyperbolic,
  logarithmic, modular, factorial/combinatoric, and helper roots
- information predicates, logical folds, `NOT`, and conditionals
- bitwise family: `BITAND`, `BITOR`, `BITXOR`, `BITLSHIFT`, `BITRSHIFT`
- aggregate wrapper: `AGGREGATE(...)`, `COM.MICROSOFT.AGGREGATE(...)`
- matrix determinant: `MDETERM(...)`
- narrow probability slice: `PROB(...)`

Hard-routed env-independent slices:

- `83` engine-first slices remain in the explicit hard-route quarantine
  surface
- those slices are concentrated in:
  - string-literal text parsing: `VALUE`, `DATEVALUE`, `TIMEVALUE`,
    `NUMBERVALUE`
  - bounded scalar financial: `RATE`
  - literal-array lookup/match/index families: `MATCH`, `XMATCH`, `LOOKUP`,
    `VLOOKUP`, `HLOOKUP`, `XLOOKUP`, `INDEX`

Legacy quarantine policy:

- default-on families now route through the shared helper in
  `interpr4.cxx`, so future retirements add one warning call instead of
  another hand-copied guard block
- string-literal `DATEVALUE` / `TIMEVALUE` wrapper bodies remain deleted even
  though their root slices are still env-`off` hard-routes rather than
  default-on families

## Current State

Today:

- the shared compiler, token model, workbook model, FODS loader, evaluator,
  dependency snapshot, invalidation planning, and recalc planning are
  engine-owned
- the `InterpretTail` seam is real production code, not a test-only oracle
- `DBG_UTIL` builds default to `observe` when the rollout env var is unset
- `authority` mode authoritatively bypasses `ScInterpreter` for supported
  promoted families
- the env-`off` hard-route boundary now covers an eighty-three-slice
  quarantine cluster, while logical constants, logical folds, `NOT`,
  conditionals, scalar-root formulas, text utility, formula text,
  conversion, information predicates, round-family roots, significant
  rounding, math-scalar roots, bitwise, aggregate, matrix determinant,
  and `PROB` now also have family-local default-on rollout paths
- the full replay corpus now has a true all-formula live-routing denominator
- replay-imported promoted formulas now reach the seam broadly, and bounded
  top-level `INDEX` / `XLOOKUP` slice results now stay inside it
- the bounded selector cluster `CHOOSECOLS` / `CHOOSEROWS` is now admitted on
  the live unique surface, reducing the dominant `unknown` unsupported-function
  bucket from `130` to `108`
- the new `matrix_math` / `MDETERM` and narrow `PROB(...)` slices now reduce
  that same `unknown` bucket further from `108` to `95`
- the latest deliberate underlying math-feeder expansion now delegates a
  bounded scalar-math family beneath comparison-helper ranges
- the earlier bounded default/approximate `MATCH` plus omitted/approximate
  extended-match work finished the hard-route frontier, and later cleanup
  converted part of that older surface into default-on families
- within the current families, the semantically distinct env-independent
  literal-array hard-route surface is now effectively exhausted
- new hard-route widening is now frozen unless it removes a live fallback
  reason or a live mismatch bucket

Still not true:

- broad wrapper retirement is now meaningfully underway, not just a handful
  of narrow milestones: after the earlier logical-constant, date/time-value,
  formula-text, conversion, numeral-conversion, `ROUNDSIG`, bitwise,
  `MDETERM`, `AGGREGATE`, `PROB`, text-utility, information-predicate, and
  logical/conditional retirements, the latest push also retires the
  scalar-root, `ERROR.TYPE`, round-family, and broad math-scalar wrapper
  clusters, and the latest safe scalar statistical relocation now also retires
  the dedicated standard-normal, exponential, gamma-inverse, permutation,
  Weibull, and `STANDARDIZE` wrappers, and the latest compat-heavy
  statistical-distribution pass now also retires the dedicated chi-square,
  chi, gamma-distribution, Student-t, F-distribution, chi-square inverse,
  Student-t inverse, F inverse, and chi inverse wrappers, and the latest
  mechanical relocation wave collapses the legacy statistical/test,
  forecasting, byte-text, and web wrapper declarations into internal helper
  paths
- the first real interpreter opcode retirement has now landed through engine
  authority, but the broader legacy opcode subsystem still remains
- the dominant retained live blocker is no longer unsupported function or
  live fallback on the validated standing corpus: both are now at `0`, so
  the next ceiling is the unseen live surface plus further Calc-path
  retirement
- the live authoritative-match north-star now sits at
  `50382 / 50,661` (`99.4493%`) on the replay corpus
- that gain now includes the earlier imported-root host-truth alignment work,
  the supported-unknown-root promotion pass that re-homed `NA`, `IMREAL`,
  `IMAGINARY`, `BESSEL*`, `PRICE`, and `SUMPRODUCT` into real evaluator
  families, the latest imported stored-host-value routing pass that promoted
  the remaining high-volume unknown roots into real probe families, and the
  latest genuine text-utility retirement push that removes the dedicated
  `SEARCH`, `REGEX`, `TEXTJOIN`, `BAHTTEXT`, the `*B` byte-text wrappers, and
  `ENCODEURL` wrappers after the earlier financial-scalar relocation, bringing
  the blunt legacy wrapper metric down to `100`; the latest `ocBad`
  retirement then deletes the dedicated `ScBadName()` fallback and brings the
  honest metric down to `99`; the latest parallel-lane retirements then
  delete the one-liner `ScNoName` and `ScCount` / `ScCount2` wrappers whose
  bodies were single delegations to `PushError(FormulaError::NoName)` and
  `IterateParameters(ifCOUNT|ifCOUNT2)` respectively, bringing the honest
  metric down to `96`; the latest scalar/default-on
  dispatch collapse then cuts the relocated-legacy companion metric to
  `62` `pushLegacy*` lambdas in `Interpret()`, with all `96` still reachable
  from opcode dispatch
- the raw promoted replay probe remains a diagnostic surface rather than the
  retirement denominator; the live-authoritative probe now sits at
  `50382 / 50386`, with only `4` live-authoritative fallback rows left on the
  standing corpus
- a focused live-host check now shows the replay-imported whole-row
  `MATCH([.$B$150];[.$150:.$150];-1)` row evaluates to
  `FormulaError::VariableExpected`
  in Calc itself, so it is no longer treated as a confirmed live parity
  blocker
- a bounded live-host-truth pass now shows the residual replay-imported
  logical-constant band is genuine live Calc error behavior, not a stale
  cached-value artifact
- a bounded live-host-truth pass now also shows the replay-imported exact
  `VLOOKUP([.P6];[.$L$2:.$M$8];2;0)` and
  `VLOOKUP(21;[.$AM$2:.$AN$4];2;0)` rows are genuine live Calc
  `FormulaError::VariableExpected` rows, not real non-error parity blockers
- a bounded live-host-truth pass now also shows the replay-imported
  collation-sensitive exact `VLOOKUP([.M22]; [.L$11:.M$32]; 1; 0)` row is a
  genuine live Calc `FormulaError::VariableExpected` row, not a real runtime
  parity blocker
- a bounded live-host-truth pass now also shows the residual replay-imported
  `XLOOKUP("Ireland"; [.H2:.H11]; [.J2:.J11]; "")` and
  `XLOOKUP([.G14]; [.I14:.R14]; [.I15:.R16])` rows are genuine live Calc
  `FormulaError::VariableExpected` rows, not real runtime parity blockers
- a bounded live-host-truth pass now also shows the replay-imported
  `INDEX([.H13:.J19]; XMATCH([.G10]; [.G13:.G19]); XMATCH([.H10]; [.H12:.J12]))`
  row is also a genuine live Calc `FormulaError::VariableExpected` row, not a
  real runtime parity blocker
- a bounded live-host-truth pass now also shows the replay-imported
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 1)`,
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 2)`, and
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 0)` rows are
  likewise genuine live Calc `FormulaError::VariableExpected` rows, not real
  runtime parity blockers
- a bounded live-host-truth pass now also shows the replay-imported
  `DATEVALUE("Jan1, 2015")` rows are genuine live Calc
  `FormulaError::VariableExpected` rows, not real runtime parity blockers
- a bounded live-host-truth pass now also shows the replay-imported
  `MATCH(1; FREQUENCY([.I126]; [.H129:.M129]); 0)` row is also a genuine live
  Calc `FormulaError::VariableExpected` row, not a real runtime parity blocker
- the promoted replay probe is now explicitly split into raw cached-workbook
  parity and live-target filtered parity: `300` rows are live-reachable and
  authoritative under seam-off direct legacy interpretation, while the
  remaining `50086` rows are imported host-truth artifacts
- the dominant retained live bucket is now unsupported shape rather than
  unsupported function; the raw promoted buckets remain diagnostic debt, not
  the deletion-gating story
- the next runtime milestone therefore should not be defined by the imported
  replay probe anymore; it should move to quality inside the new ambient live
  traffic and the remaining raw shadow-mismatch buckets, or to additional
  Calc-path quarantine / retirement slices

## Active Delegated Family

The live delegated evaluator family currently includes:

- `TRUE`
- `FALSE`
- `VALUE`
- `DATEVALUE`
- `TIMEVALUE`
- `NUMBERVALUE`
- `RATE`
- `ROUND`
- `ROUNDUP`
- `ROUNDDOWN`
- `ISERROR`
- `ISERR`
- `ISNUMBER`
- `ISNA`
- `ISTEXT`
- `ISNONTEXT`
- `ISBLANK`
- `CONCATENATE`
- `CONCAT`
- `CLEAN`
- `CHAR`
- `CODE`
- `UNICHAR`
- `UNICODE`
- `UPPER`
- `LOWER`
- `PROPER`
- `ASC`
- `JIS`
- `LEN`
- `LEFT`
- `RIGHT`
- `T`
- `EXACT`
- `FISHER`
- `FISHERINV`
- `GAUSS`
- `PHI`
- `GAMMA`
- `GAMMALN`
- `ERF`
- `ERFC`
- `POISSON`
- `POISSON.DIST`
- `BINOMDIST`
- `BINOM.DIST`
- `BINOM.DIST.RANGE`
- `B`
- `BETADIST`
- `BETA.DIST`
- `AND`
- `OR`
- `XOR`
- `NOT`
- `MATCH`
- `XMATCH`
- `LOOKUP`
- `VLOOKUP`
- `HLOOKUP`
- `XLOOKUP`
- `INDEX`
- bounded numeric aggregates:
  `SUM`, `PRODUCT`, `SUMSQ`, `AVERAGE`, `DEVSQ`, `MULTINOMIAL`,
  `SUMX2MY2`, `SUMX2PY2`, `SUMXMY2`
- bounded ranked statistical helpers:
  `QUARTILE`, `QUARTILE.INC`, `QUARTILE.EXC`,
  `PERCENTRANK`, `PERCENTRANK.INC`, `PERCENTRANK.EXC`
- bounded scalar-math helpers under comparison ranges:
  `ABS`, `PI`, trig / inverse-trig / hyperbolic variants, scalar rounding
  variants, bitwise helpers, `POWER`, `LOG`, `EXP`, `MOD`, `TRUNC`,
  `FACT`, `GCD`, `LCM`, and related aliases
- bounded `IFERROR(...)` / `IFNA(...)` wrappers around promoted roots

## Scope Policy

The active roadmap is evaluator migration.

Further computational-substrate widening is out of scope unless it directly:

- removes an `InterpretTail` fallback reason
- removes an `InterpretTail` mismatch class
- unlocks required host access for a promoted evaluator family

Residual substrate frontier items that do not satisfy one of those bars are
historical reference material, not active roadmap.

## Recommended Next Pass

The next pass is now the `RpnEvaluator` subsystem initiative:

1. complete the host-boundary audit and lock the minimal host contract for
   full engine-side RPN evaluation
2. build the engine stack-value model and typed coercion layer
3. move arithmetic, concat, comparison, and unary operator dispatch into the
   engine
4. move jump/control-flow semantics into the engine RPN loop
5. only then resume broader leaf-function retirement against those engine
   primitives
6. keep `unsupported_function=0` and `fallback=0` as regression guards while
   this subsystem work lands

## References

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md](architecture/COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md](architecture/COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md)
- [archive/interpret_tail/](archive/interpret_tail/)
- [archive/pre_pivot_substrate/](archive/pre_pivot_substrate/)
