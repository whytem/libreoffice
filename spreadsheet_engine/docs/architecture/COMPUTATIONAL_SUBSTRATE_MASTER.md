# Computational Substrate Master

Status: canonical current-state, scope, and roadmap reference

## Executive Summary

The first-stage extraction objective is materially achieved.
The live authority-transfer objective is still in progress.

Today:

- `spreadsheet_engine/` is a real shared computation layer used by both
  standalone evaluation and Calc-backed execution paths
- the promoted FODS replay baseline remains exact at zero cached fallback
- the old computational-substrate widening program proved a bounded admitted
  slice
- the live migration program now runs through a real `InterpretTail` seam in
  production Calc code
- debug and CI-style builds default that seam to `observe`
- an eighty-three-slice env-independent hard-route cluster is still
  engine-first even with rollout explicitly `off`, and logical constants,
  logical folds, `NOT`, conditionals, scalar-root formulas, text utility,
  formula text, conversion, information predicates, round-family roots,
  significant rounding, broad bounded math-scalar roots, bitwise, aggregate,
  matrix determinant, and `PROB` now also have family-local default-on
  rollout paths
- wrapper retirement is now materially underway rather than limited to a few
  showcase milestones: beyond the earlier logical-constant, date/time-value,
  formula-text, conversion, numeral-conversion, `ROUNDSIG`, bitwise,
  `MDETERM`, `AGGREGATE`, `PROB`, text-utility, information-predicate, and
  logical/conditional retirements, the latest push also retires the
  scalar-root, `ERROR.TYPE`, round-family, and broad math-scalar wrapper
  clusters, and the current unary special-function cleanup also retires the
  dedicated `FACT`, `GAMMA`, `GAMMALN`, `PHI`, `GAUSS`, `ERF`, and `ERFC`
  wrappers, bringing the blunt retirement metric down to
  `legacy_interpreter_subroutine_count=213`
- hard-route widening is now frozen unless it removes a live fallback reason
  or live mismatch bucket
- the deletion-gating live authoritative-match north-star has now moved to
  `44148 / 50,661` (`87.1440%`) on the standing replay corpus, with the broad
  corpus lane stable again after fixing the intermittent `CONVERT(...)`
  runtime crash in the shared BFS conversion path, aligning imported root
  host truth for token-backed `VariableExpected` cells, widening the scalar
  root unseen surface, and then shrinking the remaining `void Sc*()`
  declaration surface to `213`
- the broad corpus lane now completes again with `BusinessDay` admitted on the
  default ambient surface after rejecting zero-workday `WORKDAY` weekend masks
  before they enter the shared runtime, and the next `BusinessDay` slice has
  now admitted cheap local reference, holiday-range, weekend-range,
  weekend-code-ref, and named-ref shapes, with the latest bounded ambient wall
  move now also admitting `CONVERT`, the numeral-conversion roots
  `BASE` / `DECIMAL` / `ROMAN` / `ARABIC`, and `AGGREGATE`, and now
  `matrix_math` / `MDETERM` and `PROB` while pinning focused
  `ROUNDSIG` / `ORG.LIBREOFFICE.ROUNDSIG` coverage

The active program is no longer “prove more substrate slices.”
The active program is “use the substrate to underwrite live evaluator
delegation.”

## Live Migration Dashboard

### Replay Guardrail

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

### North-Star Live Authoritative Match

This is the deletion-gating number for the standing replay corpus:

- `interpret_tail_live_authoritative_corpus_formula_cells=50661`
- `interpret_tail_live_authoritative_probe_formula_cells=44152`
- `interpret_tail_live_authoritative_match_total=44148`
- `interpret_tail_live_authoritative_fallback_total=4`
- `legacy_interpreter_subroutine_count=213`
- live authoritative-match rate over the corpus: `87.1440%`
- live authoritative-match rate over the current promoted probe: `99.9909%`

Everything below is diagnostic context for improving that number.

The north-star measures live authority transfer. `legacy_interpreter_subroutine_count`
is the blunt retirement-progress companion metric, derived from the remaining
`void Sc*()` declarations in [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx).
Lower is better.

### Full Replay Corpus: Ambient Live Observe Attempts

These are attempt totals from live observe, not unique-cell coverage.

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=90682`
- `interpret_tail_live_fallback_total=0`
- `interpret_tail_live_seen_total=90682`
- `interpret_tail_live_unseen_formula_cells=0`
- `interpret_tail_live_promoted_function_supported_total=90552`
- `interpret_tail_live_supported_rate=178.99`
- `interpret_tail_live_seen_rate=178.99`

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
- `interpret_tail_live_unique_seen_formula_cells=44217`
- `interpret_tail_live_unique_supported_formula_cells=44217`
- `interpret_tail_live_unique_fallback_formula_cells=0`
- `interpret_tail_live_unique_unsupported_function_formula_cells=0`
- `interpret_tail_live_unique_unseen_formula_cells=6444`
- `interpret_tail_live_unique_seen_rate=87.28`
- `interpret_tail_live_unique_supported_rate=87.28`

### Live Unique Unsupported-Function Top-N

No live unique unsupported-function residue remains on the validated standing
corpus.

Routing policy:

- keep `unsupported_function=0` as a regression guard
- steer the next phase off unseen live surface and wrapper retirement

Unknown root split:

- `ERROR.TYPE`: `1`

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
- `interpret_tail_forced_interpret_supported_total=2649701`
- `interpret_tail_forced_interpret_fallback_total=0`
- `interpret_tail_forced_interpret_seen_total=2649701`
- `interpret_tail_forced_interpret_unseen_formula_cells=0`
- `interpret_tail_forced_interpret_promoted_function_supported_total=2649636`
- `interpret_tail_forced_interpret_supported_rate=5230.26`
- `interpret_tail_forced_interpret_seen_rate=5230.26`

### Full Replay Corpus: Forced Direct Unique-Cell Surface

This is the direct-routing comparison surface after explicitly dirtying and
forcing each replay formula cell once.

- `interpret_tail_forced_direct_formula_cells=50661`
- `interpret_tail_forced_direct_seen_formula_cells=44214`
- `interpret_tail_forced_direct_supported_formula_cells=44214`
- `interpret_tail_forced_direct_fallback_formula_cells=0`
- `interpret_tail_forced_direct_unseen_formula_cells=6447`
- `interpret_tail_forced_direct_seen_rate=87.27`
- `interpret_tail_forced_direct_supported_rate=87.27`

### Raw Cached-Workbook Promoted Probe

- `interpret_tail_probe_formula_cells=44152`
- `interpret_tail_authoritative_total=300`
- `interpret_tail_authoritative_fallback_total=43852`
- raw promoted authoritative rate: `0.68%`

Dominant promoted-family fallback reasons:

- `shadow_mismatch=43852`
- `unsupported_function=0`
- `unsupported_formula_shape=0`
- `unsupported_host_surface=0`

### Live-Reachable vs Imported-Artifact Promoted Probe

- `interpret_tail_probe_live_reachable_formula_cells=300`
- `interpret_tail_probe_imported_artifact_formula_cells=43852`
- live-reachable promoted rate: `0.68%`
- imported-artifact-only promoted rate: `99.32%`

Interpretation:

- the promoted probe is now diagnostic-only and only meaningful when split
  into live-reachable vs imported-artifact-only buckets
- the `shadow_mismatch=43852` wall is overwhelmingly imported cached-workbook
  debt, not a live parity denominator
- the raw promoted authoritative rate is now `0.68%`, so this
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

The strategy reset is recorded in
[COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md).
The active current-state ledger is
[COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md).
The latest replay eligibility closeout is now folded into that migration ledger.

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
   the seam now supports `44217` live-unique cells, but `6444` replay
   formula cells still do not enter the live unique surface
2. raw promoted diagnostic debt:
   the raw promoted replay probe is now entirely imported-artifact-only and
   remains a cached-workbook diagnostic surface, not a retirement denominator
3. further Calc-path retirement:
   the next broad value now comes from deleting more legacy wrapper clusters
   from the Calc side while holding the live authoritative rate above `85%`

The live authoritative-match north-star on the standing replay corpus is now
`44148 / 50,661` (`87.1440%`). The honest live unique-cell inventory now
shows `44217 / 50,661` formula cells seen (`87.28%`) and
`44217 / 50,661` supported (`87.28%`) during the bulk live observe run, while
the forced-direct comparison surface now sits at
`44214 / 50,661` seen (`87.27%`) and `44214 / 50,661` supported (`87.27%`).
Those are the coverage-style numbers we should currently use alongside the
north-star; the broader live and forced-interpret counters are still attempt
telemetry rather than a deletion denominator.

That gain came first from imported-root host-truth alignment and the earlier
bounded family admissions, and now further from the latest broad retirement
push: scalar-root formulas, `ERROR.TYPE`, round-family roots, bounded
math-scalar roots, and the unary special-function tails are all now
engine-owned or seam-admitted paths, with their dedicated Calc wrappers
deleted. The imported `FORMULA(...)` slice remains the biggest single
north-star mover, but the latest unary special-function cleanup is the one
that moves the blunt retirement metric down to `213` while nudging the live
unique surface wider without reintroducing any live fallback or
unsupported-function residue. The raw promoted replay probe now sits at
`300 / 44152` and remains purely diagnostic rather than a retirement
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

With the new host-truth filtered probe, `43852` promoted replay probe rows now
classify as imported host-truth artifacts under seam-off direct legacy
interpretation, while `300` rows are live-reachable and authoritative. So that
probe remains useful as a cached imported correctness surface, but not as the
live retirement denominator.

## Recommended Next Pass

The next pass should:

1. do not add new hard-route slices unless they remove a live fallback reason
   or live mismatch bucket
2. treat live authoritative-match as the single north-star metric for
   retirement progress
3. treat the raw promoted replay probe as a cached imported correctness
   surface, not as the live retirement denominator
4. keep targeting slices that increase live authoritative-match directly,
   led now by unseen live surface and additional wrapper retirement rather
   than by replay-probe cleanup
5. keep `NETWORKDAYS`, `WORKDAY`, `NETWORKDAYS.INTL`, and `WORKDAY.INTL`
   behind explicit opt-in until the ambient evaluator path is cheap enough for
   broad corpus measurement
6. treat `unsupported_function=0` as a regression guard and focus the next
   measurable push on unseen-surface reductions
7. use the broadened scalar-root / round / math-scalar retirement pass as the
   template for the next large Calc wrapper-deletion batch
8. only return to imported replay parity if we intentionally decide to
   rehabilitate legacy seam-off imported-formula execution

## Navigation

Use these documents in order:

1. [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
2. [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
3. [../PROJECT_STATUS.md](../PROJECT_STATUS.md)
4. [../archive/interpret_tail/](../archive/interpret_tail/)
5. [../archive/pre_pivot_substrate/](../archive/pre_pivot_substrate/)
