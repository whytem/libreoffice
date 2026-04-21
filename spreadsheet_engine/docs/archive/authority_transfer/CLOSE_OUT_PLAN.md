# RPN Evaluator Close-Out Plan

Status: archived working plan from the pre-relocation closeout phase,
superseded by
[../../architecture/STACK_MACHINE_RELOCATION_BACKLOG.md](../../architecture/STACK_MACHINE_RELOCATION_BACKLOG.md)
for current implementation order

Execution of the current strategic pivot is tracked in
[../../architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](../../architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md).

## Goal

Close out the five-batch RPN Evaluator plan against its explicit Success
Criteria:

- `legacy_interpreter_subroutine_count` ≤ `40` (see current value in
  [../../PROJECT_STATUS.md](../../PROJECT_STATUS.md))
- `interp4_dispatch_legacy_lambda_count` ≤ `10` (see current value in
  [../../PROJECT_STATUS.md](../../PROJECT_STATUS.md))
- every retired opcode shows non-zero exercised engine-first runtime on a
  focused or corpus audit lane before fallback deletion
- acceptance rate ≥ `0.95` across the exercised retirement lane for each
  retired opcode family
- Live authoritative-match rate ≥ `99.5%` (see current value in
  [../../PROJECT_STATUS.md](../../PROJECT_STATUS.md))
- Host facade has stable explicit contracts for every resolution /
  iteration / materialization primitive the batches depend on
- Every retirement commit follows the `ocBad` template
- All five substrate headers present under `runtime/` (complete; Batch 5
  still needs public-header normalization)

## Shape of the work

Retirement — not admission — remains the main lever that reduces the two
headline counters, but the recent Batch-5 sweep changed the local optimum.
With the honest baseline tracked canonically in
[../../PROJECT_STATUS.md](../../PROJECT_STATUS.md), the
highest-leverage remaining surface is no longer the pure text/info and
parsing/inspection cluster inside `Interpret()` because that focused wave is
now complete. The next leverage point is the remaining host-sensitive text
tail plus the substrate-gated phases below.

The remaining admissions all gate on substrate or numerical cores
that do not yet exist. Each of Phases B–F adds one such substrate and
the admissions it unlocks; they are independent of each other and can
run in parallel worktrees.

Freeze rule for the pivot:

- no new lower-seam `tryPushEngine*` admission should land in
  `ScInterpreter::Interpret()` unless it is deletion-backed, introduces a
  required RPN subsystem primitive, or closes a measured parity gap
- every surviving lower-seam admission site should carry an explicit pivot
  rationale marker in source so the policy is reviewable and CI-checkable

## Phase A — Text/Info Retirement Wave

Status: complete on the current tree.

Executed through the focused plan in
[COMPUTATIONAL_SUBSTRATE_TEXT_INFO_RETIREMENT_PLAN.md](COMPUTATIONAL_SUBSTRATE_TEXT_INFO_RETIREMENT_PLAN.md):

- Wave A: pure text/info subset
- Wave B: parsing/inspection subset
- Wave C: close any remaining authoritative-match gaps owned by those waves
  before fallback deletion

Result:

- Wave A pure text/info subset: complete
- Wave B parsing/inspection subset: complete
- wave-owned authoritative-match gaps: complete at `0`
- `interp4_dispatch_legacy_lambda_count` moved `25` → `21`
- the remaining text surface is now predominantly the explicitly-deferred
  host-sensitive tail rather than the pure/scalar subset

## Phase B — Batch 3 tail substrate + admissions

Five focused substrate pieces, each unlocks one admission:

1. `CriteriaAggregateKind::Count2` — count non-empty including text.
   Admits `ocDBCount2`.
2. `DBCountWithMissingFieldMode` — adds the `bMissingField` branch to
   the DB iterator. Admits `ocDBCount`.
3. `VarianceAccumulator` — Welford-style numerically-stable
   accumulator. Admits `ocDBStdDev` / `StdDevP` / `Var` / `VarP` via
   `planDatabaseVarianceAggregate`.
4. `planCountEmptyRange` on a streaming range-iteration host
   primitive. Admits `ocCountEmptyCells`.
5. `planDBGet` — uniqueness-enforcing scalar return. Admits `ocDBGet`.

Gate: known-regressions baseline unchanged; standalone engine
`query_tests` cover every new planner.

## Phase C — Batch 4 matrix numerical cores

Add two engine functions:

1. `semath::evaluateMatrixMultiply(left, right)` — straight matrix
   product, documented summation order to match legacy.
2. `semath::evaluateMatrixInverse(matrix)` — LUP decomposition with the
   same singular-matrix threshold as `ScMatInv`.

Expose them as `planMatrixMultiply` and `planMatrixInverse` in
`RpnMatrix.hxx`. Admit `ocMatMult` and `ocMatInv` for svMatrix inputs.

Gate: standalone engine `matrix_tests` grow with parity fixtures
against a known-answer set (identity, 2×2, 3×3, near-singular).
Numerical epsilon policy documented.

Retroactively unblocks the "Batch 4B widens Batch 1 `ocIfJumpNotMatrix`"
note in the original plan.

## Phase D — Host facade: reference-to-matrix materialization

Adds the single Host primitive that unblocks every
currently-deferred matrix-consuming admission with a range argument.

- `materializeRangeToMatrix(CellRange) → MatrixOperand`

Widen `tryPlanEngineTranspose` / `…MatrixDeterminant` /
`…MatrixMultiply` / `…MatrixInverse` / `…Index` matrix-return form /
SUMPRODUCT family to accept range tokens by calling this primitive
before `convertMatrixRefToMatrixOperand`.

Gate: existing admissions keep their acceptance rate; decline count
for range inputs drops to near-zero.

## Phase E — Batch 4 regression / forecast substrate

Three independent numerical cores, each in its own header:

1. `LinestEngine.hxx` — QR decomposition for LINEST / LOGEST / TREND.
2. `ForecastEngine.hxx` — linear forecast + Fourier.
3. `ForecastEtsEngine.hxx` — ETS state machine. Statistics variant
   (`ETS.STAT.*`) deferred to a follow-up commit.

Admit `ScLinest`, `ScLogest`, `ScForecast`, `ScFourier`, `ScGrowth`,
`ScTrend`. `ScForecast_Ets*` admitted only in the non-statistics
variants initially.

Gate: corpus-replay diff-free on these functions vs. legacy within
documented epsilon.

## Phase F — Batch 5 substrate and admissions

Build `runtime/RpnSpill.hxx`. Add Host facade primitives:

- `allocateSpillRange(anchor, dimensions) → CellRange | SpillError`
- `checkSpillCollision(range) → bool`
- `markArrayFormulaBounds(range)`

Two admission sub-phases:

- 5A: simple-shape — `FILTER`, `SORT`, `SORTBY`, `UNIQUE`, `TAKE`,
  `DROP`.
- 5B: shape-reshaping — `HSTACK`, `VSTACK`, `CHOOSECOLS` /
  `CHOOSEROWS`, `EXPAND`, `TOCOL` / `TOROW`, `WRAPCOLS` / `WRAPROWS`,
  `TEXTSPLIT`.

Engine allocator emits `#SPILL!` on collision — new behavior, not a
migration of existing behavior.

Gate: allocator parity tests + a live-corpus sample containing
dynamic-array workbooks passes.

## Phase G — Second retirement pass + wrapper-cleanup

Once Phase B–F admissions have corpus validation, run a second
retirement sweep covering everything newly admitted. In parallel,
continue piecewise wrapper-cleanup for any family now at engine
parity that is not a batch member.

Expected delta: `legacy_interpreter_subroutine_count` → ~40;
`interp4_dispatch_legacy_lambda_count` → ~10. **Headline metrics hit their targets.**

## Phase H — Known-regressions baseline to zero

The 12 failures remaining in `CALC_TEST_KNOWN_REGRESSIONS.md` are
a mix of true substrate gaps (MATCH binary-search on mixed numeric /
text data, structured-ref implicit intersection, MODE tie-breaker
ordering) and test-expectation mismatches. Bisect each, fix root
cause or retire the expectation, remove from the list one at a time.

Gate: baseline = 0.

## Phase I — Host facade contract documentation

Write `spreadsheet_engine/docs/architecture/HOST_FACADE_CONTRACTS.md`
covering: address resolution, named / external / DB range
resolution, range iteration, regex mode, spill allocation, matrix
materialization. This is the closeout deliverable for the plan's
"stable explicit contracts" criterion.

## Sequencing

```
Phase A ─── runs throughout, metric lever
Phase B (Batch 3 tail)      ─┐
Phase C (matrix cores)      ─┤
Phase D (host facade)       ─┼─ Phase G (retire wave 2) ─── Phase I (docs)
Phase E (regression)        ─┤
Phase F (Batch 5)           ─┘
Phase H (gate to zero) ─── runs throughout
```

Phases B, C, D, E, F are independent substrate tracks and can run in
parallel worktrees. Phase A, G, H, I are sequenced in the main
worktree.

## Risk-sized effort estimate

| Phase | Effort | Risk |
|---|---|---|
| A (retirement) | 1–2 days | Low; highest ROI per commit |
| B (Batch 3 tail) | 3–5 days | Low-medium; variance numeric subtlety |
| C (matrix cores) | 3–5 days | Medium; numerical parity epsilon |
| D (host facade) | 2–3 days | Medium; touches Host interface |
| E (regression/forecast) | 1–2 weeks | High; three numerical cores |
| F (Batch 5) | 1–2 weeks | Highest; new behavior, not migration |
| G (retire wave 2) | 3–5 days rolling | Low |
| H (gate to zero) | 3–7 days | Medium; needs root-cause work |
| I (facade doc) | 2–3 days | Low |

## Closeout exit gate

All nine phases complete when every Success Criterion in
[COMPUTATIONAL_SUBSTRATE_RPN_BATCH_EXECUTION_PLAN.md](COMPUTATIONAL_SUBSTRATE_RPN_BATCH_EXECUTION_PLAN.md)
is met and `CALC_TEST_KNOWN_REGRESSIONS.md` is empty.
