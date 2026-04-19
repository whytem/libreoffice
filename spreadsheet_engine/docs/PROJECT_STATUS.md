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
- `legacy_interpreter_subroutine_count=96`
- `interp4_dispatch_legacy_lambda_count=62`
- `interp4_dispatch_legacy_dispatch_target_count=62`
- `interp4_dispatch_legacy_call_count=80`
- `interp4_dispatch_engine_attempt_count=41`
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
(`runtime/RpnControlFlow.hxx`) and its first admission: `ocIf` with a scalar
condition now tries the engine-native `planIfBranch` inside `Interpret()`
before falling back to `ScIfJump`. Reference, matrix, external-ref, string,
and jump-matrix conditions defer explicitly to the legacy path, which still
owns the matrix-frame `JumpMatrix` protocol until Batch 4 lands. The three
new `controlflow_engine_*` runtime totals are published above so the batch
has its own accounting independent of the scalar-operator counters; they
stay at `0 / 0 / 0` on the live corpus because the upstream seam captures
virtually all IF traffic before reaching `Interpret()`, matching the
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
as the control-flow counters. Remaining Batch 2 members are now narrow:
`ocIndirect` is already engine-authoritative via
`seindirectexec::resolveIndirectReference`; `ocMultiArea` is a trivial
`ScUnionFunc` wrapper with no computation to migrate; INDEX matrix-return
form and wider ADDRESS parameter combinations are gated on the Batch 4
matrix-materialization contract. Batch 2 is therefore treated as
substantively complete; the RPN evaluator initiative advances to Batch 3
(criteria / database).

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
`ocCountEmptyCells` is intentionally deferred until the engine has a
streaming range-iteration primitive; materializing a range into a
`std::vector` just to count empties would be a perf regression vs. the
legacy `ScCellIterator`.

Batch 4 substrate (`runtime/RpnMatrix.hxx`) has landed with three
admissions so far. The pure-scalar constructors `ocMatrixUnit`
(MUNIT) and `ocMatSequence` (SEQUENCE) route through
`serpn::planIdentityMatrix` / `planSequenceMatrix`: they produce
matrices from scalar-only arguments, so no host-side range
materialization is needed. The third admission, `ocMatTrans`
(TRANSPOSE), exercises the matrix-consuming path for in-memory
`svMatrix` tokens via `serpn::planTranspose`. Two bridges span the
host boundary: `convertMatrixOperandToMatrixRef` copies the engine's
`MatrixOperand` (row-major `std::vector<CellValue>`) into an
`ScMatrixRef` for `PushMatrix`, and the new
`convertMatrixRefToMatrixOperand` does the reverse for svMatrix
inputs. Reference-consuming matrix opcodes (MDETERM / MINVERSE /
MMULT / SUMPRODUCT family / regression-forecast) still need a
range-iteration primitive on the Host facade and decline to the
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
/ Text / Boolean. Remaining DB family (`ocDBCount` / `ocDBCount2` /
`ocDBProduct` / `ocDBGet` / `ocDBStdDev(P)` / `ocDBVar(P)`) still
defers — each needs its own iteration path (count-with-missing-field,
product accumulation, unique-match scalar return, variance).

Remaining Batch 1 members (`ocIfs_MS`, `ocSwitch_MS`, `ocIfError`,
`ocIfNA`, `ocLet`) already delegate to `api::logic` / `seswitchexec`
helpers inside their `pushLegacy*` lambdas. They are effectively
engine-authoritative today; re-routing them through explicit
`tryPlanEngine*` wrappers would be bookkeeping rather than migration.
The initiative doc treats them as de-facto admitted and the engine-first
dispatch work now advances Batch 2 proper.
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
