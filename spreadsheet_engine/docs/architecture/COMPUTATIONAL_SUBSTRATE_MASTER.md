# Computational Substrate Master

Status: broader narrative, scope, and forward-looking evaluator-context
reference

Canonical current state now lives in
[../PROJECT_STATUS.md](../PROJECT_STATUS.md) and
[STACK_MACHINE_RELOCATION_BACKLOG.md](STACK_MACHINE_RELOCATION_BACKLOG.md).
This document is the broader program narrative and may intentionally lag the
status snapshot when work is being regrouped. Treat the figures and queue
language below as historical context unless a section explicitly says it is
describing current policy.

## Executive Summary

The first-stage extraction objective is materially achieved.
The authority-transfer pivot is closed on the current tree.
The stack-machine relocation backlog is closed on the current tree.
The next active frontier is deliberate evaluator expansion, not backlog
cleanup.

Today:

- `spreadsheet_engine/` is a real shared computation layer used by both
  standalone evaluation and Calc-backed execution paths
- the intended long-term production path is
  `ScFormulaCell::InterpretTail() -> tryEvaluateFormula() -> FormulaEvaluator -> RpnEvaluator`
- the authority-transfer and relocation-closeout questions are now settled
  for the current tree; follow-on work is deliberate evaluator widening over
  retained host-owned surface
- the relocation closeout record lives in
  [STACK_MACHINE_RELOCATION_BACKLOG.md](STACK_MACHINE_RELOCATION_BACKLOG.md)
- the current host-boundary inventory lives in
  [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md)
- the forward-looking evaluator initiative lives in
  [COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md](COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md)
- closed migration-era plans, ledgers, and slice reports now live under
  [../archive/authority_transfer/](../archive/authority_transfer/)

The active program is no longer “prove more substrate slices.”
The active program is “widen engine ownership deliberately while preserving
parity and keeping retained Calc shells intentional.”

## Historical Transition Dashboard

The detailed metrics below are retained as transition-era diagnostic context.
For the current operating view, use
[../PROJECT_STATUS.md](../PROJECT_STATUS.md) and
[STACK_MACHINE_RELOCATION_BACKLOG.md](STACK_MACHINE_RELOCATION_BACKLOG.md).

### Replay Guardrail

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

### North-Star Live Authoritative Match

The canonical keyed dashboard snapshot now lives only in
[PROJECT_STATUS.md](../PROJECT_STATUS.md).

This note intentionally keeps only explanatory context. Refresh the current
numbers from the corpus harness before using this document for planning or
triage.

The north-star measures live authority transfer. `legacy_interpreter_subroutine_count`
is the blunt retirement-progress companion metric, derived from the remaining
`void Sc*()` declarations in [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx).
Lower is better. `interp4_dispatch_legacy_lambda_count` is the relocated-legacy
companion metric for [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx):
it counts `pushLegacy*` lambdas that still compute through Calc even after
wrapper deletion. If wrapper count falls while lambda count does not, we are
shuffling implementation inside Calc rather than moving authority into the
standalone engine. The quarantine audit currently shows `62 / 62`
dispatch-reachable lambdas warning when reached. This latest drop came from
moving the compatibility-heavy statistical / aggregate / test / growth block
out of `Interpret()` and into
[InterpreterCompatDispatch.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpreterCompatDispatch.hxx),
so it is a real reduction in the relocated legacy surface rather than another
wrapper-count-only cleanup. `interp4_dispatch_engine_attempt_count` is the
static companion for the first engine-first dispatch work inside `Interpret()`:
it counts dispatch cases that now try the standalone engine first. The runtime
totals tell us whether replay traffic is actually using that path. Today they
are still `0 / 0 / 0` on the standing live corpus, but the core-forced
full-legacy replay lane now reports `602 / 602 / 0`, so the dispatch bridge is
carrying the entire residual classic tail when the classic interpreter is
deliberately exercised. That movement now comes from two engine-first slices:
`ocBad` succeeds across the full `506` error-literal rows, and the `ocRange`
audit proved that the remaining `96` classic `Range` rows were really
bracketed ODF error-literal syntax like `=[.OF:.ERR]:502`, not true
reference-range work. Those rows now reroute through the engine-backed
bad-literal path, and the legacy `ScBadName()` fallback is now deleted, so
`ocBad` is the first opcode that no longer coexists with an alternate Calc
implementation. The classic opcode census still shows `Bad=506` and
`Range=96`, anchored by formulas like `=of:#N/A` and `=[.OF:.ERR]:502`,
because the census counts opcode entry before the switch decides whether
engine or legacy computes the result. The focused `OFFSET(...):OFFSET(...)`
proof still shows valid dynamic range construction already succeeds through the
dedicated range path. So the next gap is no longer faux-range error syntax; it
is the broader real reference and control/matrix substrate.

### Full Replay Corpus: Ambient Live Observe Attempts

These are attempt totals from live observe, not unique-cell coverage.

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=1956550`
- `interpret_tail_live_fallback_total=0`
- `interpret_tail_live_seen_total=1956550`
- `interpret_tail_live_unseen_formula_cells=0`
- `interpret_tail_live_promoted_function_supported_total=1956434`
- `interpret_tail_live_supported_rate=3862.04`
- `interpret_tail_live_seen_rate=3862.04`

Dominant ambient fallback reasons:

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

No live unique unsupported-function residue remains on the validated standing
corpus.

Routing policy:

- keep `unsupported_function=0` as a regression guard
- steer the next phase off unseen live surface and wrapper retirement
- prioritize true engine admissions that reduce the relocated-legacy lambda
  surface instead of growing `Interpret()` further

Unknown-surface tail:

- `parse_failure`: `14` formula cells, `12` unseen
- `root:array_constant`: `10` formula cells, `0` unseen
- `COM.MICROSOFT.COVARIANCE.P`: `7` formula cells, `0` unseen

The new unknown-root samples closed the ambiguity around the old `operator:+`
bucket: the unseen rows were imported `TODAY() + n` date-offset formulas in
`sequence.fods`. The engine now materializes `TODAY()` as a scalar child inside
promoted scalar-root expressions, so that bucket is gone without widening root
`TODAY()` into a standalone delegated family yet.
- `COVAR`: `7` formula cells, `0` unseen
- `PEARSON`: `7` formula cells, `0` unseen
- `SHEET`: `7` formula cells, `0` unseen

That now sharpens the next routing policy:

- the imported-root host-truth pass now treats token-backed
  `VariableExpected` as authoritative live host truth at the seam
- that clears the prior live-authoritative fallback walls for
  `statistical_distribution`, `text_utility`, `lookup`, `conversion`,
  `aggregate`, `round`, `calendar_utility`, `match`, `vlookup`, `xlookup`,
  and adjacent imported-root families
- the remaining live unique unsupported-function wall is now `0`, so the next
  bounded routing work is no longer driven by unsupported-function admission;
  it is driven by unseen live surface and retirement opportunities

### Full Replay Corpus: Forced Interpret Observe Attempts

These are also attempt totals. The new forced-direct inventory below is the
honest unique-cell surface for direct routing.

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=2660369`
- `interpret_tail_forced_interpret_fallback_total=0`
- `interpret_tail_forced_interpret_seen_total=2660369`
- `interpret_tail_forced_interpret_unseen_formula_cells=0`
- `interpret_tail_forced_interpret_promoted_function_supported_total=2660304`
- `interpret_tail_forced_interpret_supported_rate=5251.32`
- `interpret_tail_forced_interpret_seen_rate=5251.32`

### Full Replay Corpus: Forced Direct Unique-Cell Surface

This is the direct-routing comparison surface after explicitly dirtying and
forcing each replay formula cell once.

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

Dominant promoted-family fallback reasons:

- `shadow_mismatch=50086`
- `unsupported_function=0`
- `unsupported_formula_shape=0`
- `unsupported_host_surface=0`

### Live-Reachable vs Imported-Artifact Promoted Probe

- `interpret_tail_probe_live_reachable_formula_cells=300`
- `interpret_tail_probe_imported_artifact_formula_cells=50086`
- live-reachable promoted rate: `0.61%`
- imported-artifact-only promoted rate: `99.39%`

Interpretation:

- the promoted probe is now diagnostic-only and only meaningful when split
  into live-reachable vs imported-artifact-only buckets
- the `shadow_mismatch=50086` wall is overwhelmingly imported cached-workbook
  debt, not a live parity denominator
- the raw promoted authoritative rate is now `0.61%`, so this
  surface remains useful for diagnostics but not for retirement steering

### Engine-Authoritative Families

Default-on families:

- logical constants
- formula text
- conversion family
- significant rounding
- bitwise family
- aggregate wrapper
- matrix determinant
- narrow probability slice

Hard-routed env-independent slices:

- `83` slices remain in the explicit hard-route quarantine surface
- they are concentrated in text parsing, bounded `RATE`, and the literal-array
  lookup/match/index families

Legacy quarantine policy:

- `interpr4.cxx` now routes all family-local default-on warnings through a
  shared helper instead of duplicating the same guard boilerplate per opcode
- string-literal `DATEVALUE` / `TIMEVALUE` wrapper bodies remain deleted even
  though their root slices are still env-`off` hard-routes rather than
  default-on families

## Strategic Position

The program now has three settled layers:

1. shared-engine extraction
2. bounded substrate proof
3. live `InterpretTail -> engine` migration

Only the third layer is the active authority-transfer roadmap.

That means:

- shared-engine extraction remains a success
- substrate widening is now secondary and conditional
- live evaluator delegation is the primary measure of progress

The strategy reset that produced the pivot is recorded in
[../archive/authority_transfer/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](../archive/authority_transfer/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md).
The completed authority-transfer handoff is recorded in
[COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md).
The relocation closeout record now lives in
[STACK_MACHINE_RELOCATION_BACKLOG.md](STACK_MACHINE_RELOCATION_BACKLOG.md).

## Next Initiative: Engine RPN Evaluator

The project has now reached the point where the remaining evaluator work is
better described as one subsystem initiative than as another sequence of leaf
function ports.

The surviving surface is dominated by:

- operator opcodes over polymorphic stack values
- jump/control-flow opcodes
- reference-producing and reference-consuming opcodes
- matrix broadcast and matrix-frame state
- criteria/database iteration
- stack/runtime state such as error and format propagation

That initiative is now tracked in:

- [COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md](COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md)
- [COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md](COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md)

The key policy change is that wrapper deletion alone is no longer treated as
equivalent to engine migration. With the relocation backlog closed, the next
forward-looking metric is deliberate reduction of retained host-owned overlap,
not another wrapper-only cleanup wave.

## Engine-Owned Today

The engine broadly owns:

- compiler and token infrastructure
- standalone workbook loading and evaluation
- dependency snapshots, invalidation planning, and recalc planning
- substantial shared runtime already consumed by Calc

On the live migration track, the engine now also owns bounded delegated
evaluation for:

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
- bounded aggregate wrapper:
  `AGGREGATE`, `COM.MICROSOFT.AGGREGATE`
- bounded matrix math:
  `MDETERM`
- narrow statistical-distribution root:
  `PROB`
- bounded `IFERROR(...)` / `IFNA(...)` wrappers around promoted roots

## Calc-Retained Today

Calc still intentionally owns:

- broad document storage and mutation
- broad listener and broadcaster residency
- broad formula evaluation outside the promoted delegated family
- token-container construction and Calc-local token plumbing
- UI, UNO, import/export, rendering, persistence, and shell integration
- external-reference and environment-sensitive host behavior

That retained shell is expected. The migration goal is targeted delegation,
not immediate whole-document replacement.

## Scope Policy

The active roadmap is evaluator migration.

New computational-substrate widening is out of scope unless it directly:

- removes a live `InterpretTail` fallback reason
- removes a live `InterpretTail` mismatch class
- unlocks host access needed by a promoted evaluator family

Historical substrate frontier items that do not meet one of those bars
remain archived reference material only.

## Biggest Remaining Blockers

The highest-value remaining blockers are now:

1. unseen live surface:
   the seam now supports `50640` live-unique cells, but `21` replay
   formula cells still do not enter the live unique surface
2. raw promoted diagnostic debt:
   the raw promoted replay probe is now entirely imported-artifact-only and
   remains a cached-workbook diagnostic surface, not a retirement denominator
3. further Calc-path retirement:
   the next broad value now comes from deleting more legacy wrapper clusters
   from the Calc side while holding the live authoritative rate above `90%`

The live authoritative-match north-star on the standing replay corpus is now
`50382 / 50,661` (`99.4493%`). The honest live unique-cell inventory now
shows `50640 / 50,661` formula cells seen (`99.90%`) and
`50640 / 50,661` supported (`99.90%`) during the bulk live observe run, while
the forced-direct comparison surface now sits at
`44773 / 50,661` seen (`88.38%`) and `44773 / 50,661` supported (`88.38%`).
Those are the coverage-style numbers we should currently use alongside the
north-star; the broader live and forced-interpret counters are still attempt
telemetry rather than a deletion denominator. The latest genuine text-utility
retirement pass brings the honest blunt Calc-wrapper metric down to `100`, and
the follow-on scalar/default-on dispatch collapse cuts the relocated-legacy
companion metric to `62` `pushLegacy*` lambdas still resident in `Interpret()`,
so wrapper deletion should not be read as full standalone-engine migration by
itself.

That gain came first from imported-root host-truth alignment and the earlier
bounded family admissions, and now further from the supported-unknown-root
promotion pass and the latest broad retirement wave: scalar-root formulas,
`ERROR.TYPE`, round-family roots, bounded math-scalar roots, the unary
special-function tails, the safe and compat-heavy statistical wrappers, and
now the broad statistical/test, forecasting, byte-text, and web wrapper
clusters are all engine-owned, seam-admitted, or mechanically relocated
paths, with the blunt retirement metric now down to `50` while the live
unique surface widens further without reintroducing any live fallback or
unsupported-function residue. The raw promoted replay probe now sits at
`300 / 50386` and remains purely diagnostic rather than a retirement
denominator.

Inside the current families, the semantically distinct env-independent
literal-array hard-route frontier is now frozen. New widening is out of scope
unless it removes a live fallback reason or live mismatch bucket.

A focused live-host check now shows the replay-imported whole-row
`MATCH([.$B$150]; [.$150:.$150]; -1)` row evaluates to
`FormulaError::VariableExpected` in Calc with the seam forced off, so that
row is no longer treated as a confirmed live parity blocker even though the
replay workbook stores a non-error expected value.

A bounded live-host-truth pass now also shows the replay-imported exact
`VLOOKUP([.P6]; [.$L$2:.$M$8]; 2; 0)` and
`VLOOKUP(21; [.$AM$2:.$AN$4]; 2; 0)` rows both evaluate to
`FormulaError::VariableExpected` in Calc with the seam forced off, so those
cached non-error workbook values are likewise no longer treated as runtime
conversion targets.

A bounded live-host-truth pass now also shows the replay-imported
collation-sensitive exact `VLOOKUP([.M22]; [.$L$11:.$M$32]; 1; 0)` row
evaluates to `FormulaError::VariableExpected` in Calc with the seam forced
off, so the residual replay `VLOOKUP` band is no longer treated as a real
runtime conversion target either.

A bounded live-host-truth pass now also shows the residual replay-imported
`XLOOKUP("Ireland"; [.$H$2:.$H$11]; [.$J$2:.$J$11]; "")` and
`XLOOKUP([.$G$14]; [.$I$14:.$R$14]; [.$I$15:.$R$16])` rows evaluate to
`FormulaError::VariableExpected` in Calc with the seam forced off, so the
residual replay `XLOOKUP` band is no longer treated as a real runtime
conversion target either.

A bounded live-host-truth pass now also shows the replay-imported
`INDEX([.H13:.J19]; XMATCH([.G10]; [.G13:.G19]); XMATCH([.H10]; [.H12:.J12]))`
row evaluates to `FormulaError::VariableExpected` in Calc with the seam
forced off, so that cached numeric workbook value is not treated as a real
runtime conversion target either.

A bounded live-host-truth pass now also shows the replay-imported
`INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 1)`,
`INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 2)`, and
`INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 0)` rows all
evaluate to `FormulaError::VariableExpected` in Calc with the seam forced
off, so that imported `INDEX` matrix-function band is not treated as a real
runtime conversion target either.

A bounded live-host-truth pass now also shows the replay-imported
`DATEVALUE("Jan1, 2015")` rows evaluate to `FormulaError::VariableExpected`
in Calc with the seam forced off, so those cached non-numeric workbook rows
are not treated as real runtime conversion targets either.

A bounded live-host-truth pass now also shows the replay-imported
`MATCH(1; FREQUENCY([.I126]; [.H129:.M129]); 0)` row evaluates to
`FormulaError::VariableExpected` in Calc with the seam forced off, so that
cached workbook non-error row is likewise not treated as a real runtime
conversion target.

With the new host-truth filtered probe, `50086` promoted replay probe rows now
classify as imported host-truth artifacts under seam-off direct legacy
interpretation, while `300` rows are live-reachable and authoritative. So that
probe remains useful as a cached imported correctness surface, but not as the
live retirement denominator.

## Recommended Next Pass

The next pass is now the `RpnEvaluator` subsystem initiative:

1. complete the host-boundary audit and define the minimal engine-facing host
   contract for full RPN evaluation
2. build the engine stack-value model and typed coercion layer
3. move arithmetic, concat, comparison, and unary operator dispatch into the
   engine
4. move control-flow opcodes (`IF`, `CHOOSE`, `LET`, matrix-aware jumps) into
   the engine RPN loop
5. only then resume broader leaf-function retirement against those engine
   primitives
6. keep `unsupported_function=0` and `fallback=0` as regression guards while
   this subsystem work lands

## Navigation

Use these documents in order:

1. [../PROJECT_STATUS.md](../PROJECT_STATUS.md)
2. [STACK_MACHINE_RELOCATION_BACKLOG.md](STACK_MACHINE_RELOCATION_BACKLOG.md)
3. [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md)
4. [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md)
5. [../archive/authority_transfer/](../archive/authority_transfer/)
6. [../archive/interpret_tail/](../archive/interpret_tail/)
7. [../archive/pre_pivot_substrate/](../archive/pre_pivot_substrate/)
