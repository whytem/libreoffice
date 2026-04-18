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
  clusters, the latest safe scalar statistical relocation now also retires the
  dedicated standard-normal, exponential, gamma-inverse, permutation,
  Weibull, and `STANDARDIZE` wrappers, and the latest compat-heavy
  statistical-distribution relocation now also retires the dedicated
  chi-square, chi, gamma-distribution, Student-t, F-distribution,
  chi-square inverse, Student-t inverse, F inverse, and chi inverse
  wrappers, the supported-unknown-root promotion pass now re-homes `NA`,
  `IMREAL`, `IMAGINARY`, `BESSEL*`, `PRICE`, and `SUMPRODUCT` into real
  evaluator families, and the latest mechanical relocation wave now
  collapses the statistical/test, forecasting, byte-text, and web
  wrapper declarations into internal helper paths, while the latest
  date/time, financial, and text mechanical relocation wave briefly brought the blunt
  retirement metric down to `legacy_interpreter_subroutine_count=50`, and the
  latest genuine standalone statistical-distribution relocation retires the
  legacy `ScB`, `ScNormDist`, `ScHypGeomDist`, `ScLogNormDist`,
  `ScLogNormInv`, `ScBetaDist_MS`, `ScBetaInv`, `ScCritBinom`,
  `ScNegBinomDist`, and `ScNegBinomDist_MS` wrappers; after restoring earlier
  rename-only wrapper name changes, the honest blunt metric now sits at
  `legacy_interpreter_subroutine_count=235`; the latest genuine date/time
  retirement pass then removed the dedicated `TODAY`/`NOW`, date-part,
  week-number, workday/networkdays, and `DATE`/`TIME`/date-difference wrappers,
  bringing the honest blunt metric down again to `legacy_interpreter_subroutine_count=185`;
  the latest genuine financial scalar retirement pass then removes the
  dedicated `ISPMT`, `PV`, `SYD`, `DDB`, `DB`, `VDB`, `PDURATION`, `SLN`,
  `PMT`, `RRI`, `FV`, `NPER`, `RATE`, `IPMT`, `PPMT`, `CUMIPMT`,
  `CUMPRINC`, `EFFECT`, and `NOMINAL` wrappers, bringing the honest blunt
  metric down further to `legacy_interpreter_subroutine_count=166`; the latest
  genuine text-utility retirement pass then removes the dedicated `SEARCH`,
  `REGEX`, `TEXTJOIN`, `BAHTTEXT`, the `*B` byte-text wrappers, and
  `ENCODEURL`, bringing the honest blunt metric to
  `legacy_interpreter_subroutine_count=100`
- hard-route widening is now frozen unless it removes a live fallback reason
  or live mismatch bucket
- the deletion-gating live authoritative-match north-star has now moved to
  `50354 / 50,661` (`99.3940%`) on the standing replay corpus, with the broad
  corpus lane stable again after fixing the intermittent `CONVERT(...)`
  runtime crash in the shared BFS conversion path, aligning imported root
  host truth for token-backed `VariableExpected` cells, promoting the
  supported unknown-root band into real evaluator families, then promoting
  the remaining high-volume imported unknown roots into real probe families,
  then genuinely retiring the dedicated date/time wrapper surface to `185`,
  and now genuinely retiring the dedicated text-utility wrapper surface
  to `100`
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
- `interpret_tail_live_authoritative_probe_formula_cells=50358`
- `interpret_tail_live_authoritative_match_total=50354`
- `interpret_tail_live_authoritative_fallback_total=4`
- `legacy_interpreter_subroutine_count=100`
- `interp4_dispatch_legacy_lambda_count=62`
- `interp4_dispatch_legacy_dispatch_target_count=62`
- `interp4_dispatch_legacy_call_count=80`
- `interp4_dispatch_engine_attempt_count=12`
- `interp4_dispatch_engine_attempted_total=0`
- `interp4_dispatch_engine_succeeded_total=0`
- `interp4_dispatch_engine_declined_total=0`
- `interp4_dispatch_legacy_quarantine_missing_dispatch_target_count=0`
- live authoritative-match rate over the corpus: `99.3940%`
- live authoritative-match rate over the current promoted probe: `99.9921%`

Everything below is diagnostic context for improving that number.

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
static companion for the first engine-opcode pilot: it counts dispatch cases in
`Interpret()` that now try the standalone engine first. The runtime totals tell
us whether replay traffic is actually using that path. Today they are still
`0 / 0 / 0` on the standing corpus, so the measurement is already doing useful
work by showing that the operator pilot has landed structurally without yet
carrying broad replay load.

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
- `interpret_tail_live_unique_seen_formula_cells=50612`
- `interpret_tail_live_unique_supported_formula_cells=50612`
- `interpret_tail_live_unique_fallback_formula_cells=0`
- `interpret_tail_live_unique_unsupported_function_formula_cells=0`
- `interpret_tail_live_unique_unseen_formula_cells=49`
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

- `operator:+`: `34` formula cells, `34` unseen
- `parse_failure`: `14` formula cells, `12` unseen
- `root:array_constant`: `10` formula cells, `0` unseen
- `COM.MICROSOFT.COVARIANCE.P`: `7` formula cells, `0` unseen
- `COM.MICROSOFT.COVARIANCE.S`: `7` formula cells, `0` unseen
- `COM.MICROSOFT.F.TEST`: `7` formula cells, `0` unseen
- `CORREL`: `7` formula cells, `0` unseen
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

- `interpret_tail_probe_formula_cells=50358`
- `interpret_tail_authoritative_total=300`
- `interpret_tail_authoritative_fallback_total=50048`
- raw promoted authoritative rate: `0.61%`

Dominant promoted-family fallback reasons:

- `shadow_mismatch=50048`
- `unsupported_function=0`
- `unsupported_formula_shape=0`
- `unsupported_host_surface=0`

### Live-Reachable vs Imported-Artifact Promoted Probe

- `interpret_tail_probe_live_reachable_formula_cells=300`
- `interpret_tail_probe_imported_artifact_formula_cells=50048`
- live-reachable promoted rate: `0.61%`
- imported-artifact-only promoted rate: `99.39%`

Interpretation:

- the promoted probe is now diagnostic-only and only meaningful when split
  into live-reachable vs imported-artifact-only buckets
- the `shadow_mismatch=50048` wall is overwhelmingly imported cached-workbook
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

The strategy reset is recorded in
[COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md).
The active current-state ledger is
[COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md).
The latest replay eligibility closeout is now folded into that migration ledger.

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
equivalent to engine migration. The leading metric for the next phase is
reduction in `interp4_dispatch_legacy_lambda_count`.

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
   the seam now supports `50612` live-unique cells, but `49` replay
   formula cells still do not enter the live unique surface
2. raw promoted diagnostic debt:
   the raw promoted replay probe is now entirely imported-artifact-only and
   remains a cached-workbook diagnostic surface, not a retirement denominator
3. further Calc-path retirement:
   the next broad value now comes from deleting more legacy wrapper clusters
   from the Calc side while holding the live authoritative rate above `90%`

The live authoritative-match north-star on the standing replay corpus is now
`50354 / 50,661` (`99.3940%`). The honest live unique-cell inventory now
shows `50612 / 50,661` formula cells seen (`99.90%`) and
`50612 / 50,661` supported (`99.90%`) during the bulk live observe run, while
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
`300 / 50358` and remains purely diagnostic rather than a retirement
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

With the new host-truth filtered probe, `50048` promoted replay probe rows now
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

1. [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
2. [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
3. [../PROJECT_STATUS.md](../PROJECT_STATUS.md)
4. [../archive/interpret_tail/](../archive/interpret_tail/)
5. [../archive/pre_pivot_substrate/](../archive/pre_pivot_substrate/)
