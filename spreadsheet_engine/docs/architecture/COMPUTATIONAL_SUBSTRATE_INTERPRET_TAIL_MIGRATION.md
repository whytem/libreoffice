# InterpretTail Engine Migration

Status: active migration ledger for `ScFormulaCell::InterpretTail -> engine evaluator`

## Purpose

This is the active current-state document for the live evaluator migration.

Use it for:

- the current delegated family
- the current routing modes
- the current live metrics
- the current hard-routed or quarantined paths
- the current next target
- the active next-slice plan
- the active next subsystem initiative

The older per-pass plan, decision, and evidence documents are retained in
[../archive/interpret_tail/](../archive/interpret_tail/) for history only.

## Current State

The migration is real, but still early.

Today:

- the seam is wired into `ScFormulaCell::InterpretTail`
- `DBG_UTIL` builds default to `observe` when the rollout env var is unset
- release-style builds still default to `off` unless explicitly opted in
- `observe`, `shadowcompare`, and `authority` are all live on the production
  Calc path
- an eighty-three-slice env-independent hard-route cluster is still
  engine-first even with rollout set to `off`, and logical constants,
  logical folds, `NOT`, conditionals, text utility, formula text,
  conversion, information predicates, significant rounding, bitwise,
  aggregate, matrix determinant, and a narrow `PROB(...)` slice now also
  have family-local default-on rollout paths
- direct scalar-root formulas and a bounded scalar utility cluster now also
  ride the live seam: arithmetic/reference/concat/comparison roots plus
  `ROUND`, `ROUNDUP`, `ROUNDDOWN`, information predicates, logical folds, and
  `NOT`
- replay-imported promoted formulas now reach the live seam on both the
  ambient and forced-interpret replay surfaces through a bounded pre-RPN
  observe bridge
- nested delegated `XLOOKUP` and `INDEX` matrix-return sources now stay inside
  the engine seam on the replay-imported promoted surface
- top-level replay-imported `INDEX` and `XLOOKUP` slice results now scalarize
  through the engine seam instead of falling out as generic host-surface
  fallout
- replay-imported named-range `VLOOKUP` and `INDEX` rows now resolve through
  the engine seam using the stored name-definition base position instead of
  falling out as generic host-surface fallout
- the latest deliberate family expansion materially raised the ambient live
  denominator instead of only refining replay-imported parity inside already-
  promoted families, and the latest routing slices now also admit bounded
  `matrix_math` / `MDETERM`, a narrow default-on `PROB(...)` slice, and a
  bounded `AGGREGATE(...)` wrapper slice

What is still not true:

- no broad default-on production rollout exists beyond the narrow
  logical-constant, logical, conditional, text-utility, formula-text,
  conversion, and information-predicate families, the narrow `ROUNDSIG`
  and bitwise scalar slices, the bounded aggregate and matrix-determinant
  slices, and the narrow `PROB(...)` slice
- sixteen narrow legacy deletion milestones have landed:
  `ScInterpreter::ScTrue()` / `ScFalse()` and the dedicated
  `ScGetDateValue()` / `ScGetTimeValue()` wrapper pair, plus the dedicated
  `ScInterpreter::ScFormula()` wrapper, and now the dedicated
  `ScInterpreter::ScConvertOOo()` wrapper, and now the dedicated
  `ScInterpreter::ScRoundSignificant()` wrapper, and now the dedicated
  `ScInterpreter::ScEuroConvert()` wrapper, and now the dedicated
  `ScInterpreter::ScBase()` / `ScDecimal()` / `ScRoman()` / `ScArabic()`
  wrappers, and now the dedicated `ScInterpreter::ScBitAnd()` /
  `ScBitOr()` / `ScBitXor()` / `ScBitLshift()` / `ScBitRshift()`
  wrappers, and now the dedicated `ScInterpreter::ScMatDet()` wrapper, and
  now the dedicated `ScInterpreter::ScAggregate()` wrapper, and now the
  dedicated `ScInterpreter::ScProbability()` wrapper, and now the dedicated
  text-utility wrappers, and now the dedicated information-predicate
  wrappers, and now the dedicated logical-fold / `NOT` / conditional
  wrappers, and now the dedicated `FACT`, `GAMMA`, `GAMMALN`, `PHI`,
  `GAUSS`, `ERF`, and `ERFC` wrappers, and now the dedicated chi-square,
  chi, gamma-distribution, Student-t, F-distribution, chi-square inverse,
  Student-t inverse, F inverse, and chi inverse wrappers, and now the
  dedicated `ScB`, `ScNormDist`, `ScHypGeomDist`, `ScLogNormDist`,
  `ScLogNormInv`, `ScBetaDist_MS`, `ScBetaInv`, `ScCritBinom`,
  `ScNegBinomDist`, and `ScNegBinomDist_MS` wrappers; broader legacy
  retirement has not started
- the full replay corpus now shows strong ambient live-seam traffic, but most
  retained fallback now lives inside the remaining ambient
  `unsupported_function` wall rather than simple denominator reach
- the next dominant live blocker is no longer simple reach; it is the
  remaining bounded-family admission work now led by the spill-heavy dynamic-
  array cluster
- no promoted family has moved from observe-only replay reach into a broader
  replay authority lane yet

## Routing Modes

The seam is controlled by
`SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR`.

Supported values:

- `observe`
- `shadow` or `shadowcompare`
- `authority`

When the env var is unset:

- `DBG_UTIL` builds use `observe`
- non-`DBG_UTIL` builds use `off`

This means developer and CI-style debug builds accumulate real seam signal by
default without changing shipping behavior.

## Delegated Family

The currently promoted live evaluator family includes:

- logical literals:
  - `TRUE`
  - `FALSE`
- text parsing:
  - `VALUE`
  - `DATEVALUE`
  - `TIMEVALUE`
  - `NUMBERVALUE`
- text utility:
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
- statistical distributions:
  - `FISHER`
  - `FISHERINV`
  - `GAUSS`
  - `PHI`
  - `GAMMA`
  - `GAMMALN`
  - `ERF`
  - `ERFC`
  - `LEGACY.CHIDIST`
  - `CHISQDIST`
  - `CHISQ.DIST`
  - `CHISQ.DIST.RT`
  - `CHISQINV`
  - `CHISQ.INV`
  - `CHISQ.INV.RT`
  - `GAMMADIST`
  - `GAMMA.DIST`
  - `TDIST`
  - `LEGACY.TDIST`
  - `T.DIST`
  - `T.DIST.2T`
  - `T.DIST.RT`
  - `TINV`
  - `T.INV`
  - `T.INV.2T`
  - `FDIST`
  - `LEGACY.FDIST`
  - `F.DIST`
  - `F.DIST.RT`
  - `FINV`
  - `LEGACY.FINV`
  - `F.INV`
  - `F.INV.RT`
  - `POISSON`
  - `POISSON.DIST`
  - `BINOMDIST`
  - `BINOM.DIST`
  - `BINOM.DIST.RANGE`
  - `B`
  - `BETADIST`
  - `BETA.DIST`
  - `PROB`
- scalar utilities:
  - `IF`
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
  - `AND`
  - `OR`
  - `XOR`
  - `NOT`
- statistical aggregates:
  - `MAX`
  - `MAXA`
  - `MIN`
  - `MINA`
  - `MEDIAN`
  - `GEOMEAN`
  - `HARMEAN`
  - `VAR*`
  - `STDEV*`
  - `LARGE`
  - `SMALL`
  - `RANK*`
- aggregate wrapper:
  - `AGGREGATE`
  - `COM.MICROSOFT.AGGREGATE`
- matrix math:
  - `MDETERM`
- bounded scalar-math helpers:
  - `FACT`
- lookup and index:
  - `MATCH`
  - `XMATCH`
  - `LOOKUP`
  - `VLOOKUP`
  - `HLOOKUP`
  - `XLOOKUP`
  - `INDEX`
- bounded wrappers:
  - `IFERROR(...)` around already-promoted roots
  - `IFNA(...)` around already-promoted roots

The admitted live input surface includes:

- literals
- single-cell references
- single-cell workbook-global names
- single-cell sheet-local names
- bounded scalar expression trees
- bounded single-area workbook-local lookup or index reads whose result stays
  scalar or single-cell

## Live Metrics

Two different denominators matter, and both are now reported.

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
- `interp4_dispatch_engine_attempted_total_core_forced_full_legacy=0`
- `interp4_dispatch_engine_succeeded_total_core_forced_full_legacy=0`
- `interp4_dispatch_engine_declined_total_core_forced_full_legacy=0`
- `sc_formula_executor_classic_interpret_total_live=0`
- `sc_formula_executor_classic_interpret_total_core_forced_full_legacy=602`
- `interp4_dispatch_legacy_quarantine_missing_dispatch_target_count=0`
- live authoritative-match rate over the corpus: `99.3940%`
- live authoritative-match rate over the current promoted probe: `99.9921%`

Everything below is diagnostic context for improving that number.

The north-star measures live authority transfer. `legacy_interpreter_subroutine_count`
is the blunt retirement-progress companion metric, derived from the remaining
`void Sc*()` declarations in [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx).
Lower is better. `interp4_dispatch_legacy_lambda_count` is the relocated-legacy
companion metric: it counts `pushLegacy*` lambdas still computing inside
[interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
after wrapper deletion. The quarantine audit currently shows `62 / 62`
dispatch-reachable lambdas warning when reached, but the absolute count makes
clear that relocation and engine migration are different kinds of progress.
This latest drop came from moving the compatibility-heavy statistical /
aggregate / test / growth block out of `Interpret()` and into
[InterpreterCompatDispatch.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpreterCompatDispatch.hxx),
so it is a real reduction in the relocated legacy surface rather than another
wrapper-count-only cleanup. `interp4_dispatch_engine_attempt_count` now tracks
the new operator pilot cases that try the standalone engine first from
`Interpret()`, and the paired runtime totals show whether replay traffic is
actually using them. On the standing corpus those runtime counters are still
`0 / 0 / 0`, and the new core-forced full-legacy replay lane also still shows
`0 / 0 / 0` for operator attempts even though it now reaches classic
`ScInterpreter::Interpret()` `602` times. So the next operator slice should be
judged by moving those runtime numbers inside the residual classic tail, not
just by growing the static case count. The first census of that tail shows
`Bad=506` and `Range=96` as the dominant classic opcodes, with sampled formulas
like `=of:#N/A` and `=of:#ERR504!`, which points the next slice toward
error-literal and residual range handling rather than more scalar operator
widening.

### Full Replay Corpus: Ambient Live Observe Attempts

These are attempt totals from live observe, not unique-cell coverage:

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=1956550`
- `interpret_tail_live_fallback_total=0`
- `interpret_tail_live_seen_total=1956550`
- `interpret_tail_live_unseen_formula_cells=0`
- `interpret_tail_live_promoted_function_supported_total=1956434`
- `interpret_tail_live_supported_rate=3862.04`
- `interpret_tail_live_seen_rate=3862.04`

Current ambient fallback reasons:

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

Interpretation:

- the seam now measures the full replay corpus
- these live observe totals are useful hotspot telemetry, but they are still
  attempt counters rather than a unique-cell retirement denominator
- the imported-root host-truth pass now treats token-backed
  `VariableExpected` as authoritative live host truth at the seam, and the
  latest scalar-root / round / math-scalar retirement push then carried the
  live unique fallback and unsupported-function residue all the way to `0`
- the latest genuine text-utility retirement pass now removes the dedicated
  `SEARCH`, `REGEX`, `TEXTJOIN`, `BAHTTEXT`, the `*B` byte-text wrappers, and
  `ENCODEURL` while preserving the same engine-authoritative live behavior,
  bringing `legacy_interpreter_subroutine_count` down from `112` to `100`
- the follow-on scalar/default-on dispatch collapse cuts the relocated-legacy
  companion metric to `62` `pushLegacy*` lambdas still present in `Interpret()`,
  with all `62` reachable from opcode dispatch; future work should reduce that
  surface rather than treating wrapper deletion alone as migration
- the ambient replay surface now includes large real traffic from
  information predicates, logical folds, round-family formulas, scalar-math
  helpers, bounded financial feeders, bounded numeric aggregates, bounded
  ranked statistical helpers, and bounded calendar/date utility helpers
- the imported direct information-predicate host-truth residue is now closed
  on the live surface
- the imported direct logical-fold residue and the imported nested-`XMATCH`
  `INDEX` residue are now also closed on the live surface
- the broad corpus lane now completes again with `BusinessDay` admitted on
  the default ambient surface after rejecting zero-workday `WORKDAY` weekend
  masks before they enter the shared runtime
- the next `BusinessDay` slice has now admitted cheap local reference,
  holiday-range, weekend-range, weekend-code-ref, and named-ref shapes, which
  moved that live family from `118 supported / 158 fallback` to
  `274 supported / 2 fallback`
- the bounded selector cluster `CHOOSECOLS` / `CHOOSEROWS` is now admitted on
  the live unique surface as `selector=62` cells, `58` supported / `4`
  fallback, and reduced the remaining live unique `unknown` wall from
  `130` to `108`
- the new bounded `matrix_math` slice now admits `MDETERM`, contributes
  `12` live unique seen cells with `10` supported and `2` fallback, and now
  also runs through a family-local default-on path with the dedicated
  `ScInterpreter::ScMatDet()` wrapper retired
- a new bounded statistical aggregate slice now admits `MAX` / `MIN`,
  `MEDIAN`, `GEOMEAN` / `HARMEAN`, and `VAR*` / `STDEV*`, while the ranked
  aggregate lane now also covers `LARGE` / `SMALL` / `RANK*`
- a new bounded criteria-aggregate slice now admits `COUNTIF` / `COUNTIFS`,
  `SUMIF` / `SUMIFS`, `AVERAGEIF` / `AVERAGEIFS`, and `MAXIFS` / `MINIFS`,
  contributing `203` live unique seen cells with `203` supported and `0`
  fallback after also closing the matrix-`IF(...)` criteria residue
- a new bounded scalar `IF(...)` family now contributes `1887` live unique
  seen cells with `1873` supported and `14` fallback
- a new bounded `text_utility` family now contributes `881` live unique seen
  cells with `859` supported and `22` fallback
- a new bounded `statistical_distribution` family now contributes `1052`
  live unique seen cells with `1049` supported and `3` fallback
- a new bounded `growth_projection` slice now contributes `10` live unique
  seen cells with `10` supported and `0` fallback, removing `GROWTH` from
  the remaining `unknown` wall
- imported `FORMULA(...)` live-host-truth alignment now contributes `14278`
  live unique seen cells with `14278` supported and `0` fallback, and is the
  biggest single north-star mover so far
- the latest bounded ambient wall move now also admits `CONVERT`, which
  contributes `308` live unique seen cells with `308` supported and `0`
  fallback, plus a bounded `AGGREGATE` slice contributing `212` live unique
  seen cells with `211` supported and `1` fallback and now running through a
  family-local default-on retirement path, plus a bounded `matrix_math`
  slice contributing `12` live unique seen cells with `10` supported and `2`
  fallback and now running through a family-local default-on retirement path,
  while focused `ROUNDSIG` /
  `ORG.LIBREOFFICE.ROUNDSIG` live-seam coverage is now pinned in the
  validation suite
- the live unique unsupported-function routing table is now explicit:
  `unknown=54`, `text_utility=19`, `conditional=11`; that ordering now drives
  the next ambient family-selection work instead of intuition
- the remaining `unknown` bucket is now split by live unique root name:
  `FORECAST.ETS.MULT=3`, `FORECAST.ETS=3`, `MODE.MULT=3`, `VSTACK=3`,
  `MODE.SNGL=2`, `COMPLEX=2`, `KURT=2`, `MODE=2`,
  `COVARIANCE.P=2`, `COVARIANCE.S=2`
- that makes the next routing policy concrete:
  `TEXTAFTER(...)` is now admitted through `text_utility`,
  `TEXTBEFORE(...)` is now admitted through `text_utility`,
  `IFS(...)` / `SWITCH(...)` are now admitted through `conditional`,
  and the spill-heavy dynamic-array cluster
  (`UNIQUE`, `SORT`, `SORTBY`, `TEXTSPLIT`, `HSTACK`) is now admitted as
  `spill_array=83` live unique cells with `73` supported / `10` fallback,
  while the broad corpus lane is stable again after fixing the intermittent
  `CONVERT(...)` runtime crash in the shared BFS conversion path, and the
  bounded `FORECAST(...)` / `INTERCEPT(...)` regression slice is now admitted
  through `statistical_distribution`, so the next bounded routing work should
  move to `MODE.MULT` / `MODE.SNGL`, `KURT`, and adjacent covariance roots
  before heavier ETS or spill-shaped work
- the main remaining ambient work is now unseen live surface plus further
  Calc-path retirement, not a live unsupported-function wall
- the live authoritative-match north-star now sits well above the `65%`
  milestone, with the standing-corpus live fallback and unsupported-function
  bands both at `0`

### Full Replay Corpus: Forced Interpret Observe Attempts

These are also attempt totals after explicitly dirtying and forcing every
replay formula cell through Calc's live `Interpret()` path:

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=2660369`
- `interpret_tail_forced_interpret_fallback_total=0`
- `interpret_tail_forced_interpret_seen_total=2660369`
- `interpret_tail_forced_interpret_unseen_formula_cells=0`
- `interpret_tail_forced_interpret_promoted_function_supported_total=2660304`
- `interpret_tail_forced_interpret_supported_rate=5251.32`
- `interpret_tail_forced_interpret_seen_rate=5251.32`

Interpretation:

- forced interpret still confirms that a much larger fraction of the corpus
  reaches the seam when explicitly dirtied and interpreted
- forced interpret is now also fully fallback-free on the validated standing
  corpus, so it has become a pure reach/telemetry lane rather than a fallback
  debugging surface

### Full Replay Corpus: Forced Direct Unique-Cell Surface

This is the direct-routing comparison surface after dirtying and forcing each
replay formula cell once, then classifying whether that formula cell was
actually seen and supported by the seam:

- `interpret_tail_forced_direct_formula_cells=50661`
- `interpret_tail_forced_direct_seen_formula_cells=44773`
- `interpret_tail_forced_direct_supported_formula_cells=44773`
- `interpret_tail_forced_direct_fallback_formula_cells=0`
- `interpret_tail_forced_direct_unseen_formula_cells=5888`
- `interpret_tail_forced_direct_seen_rate=88.38`
- `interpret_tail_forced_direct_supported_rate=88.38`

### Raw Cached-Workbook Promoted Probe

This is the promoted-family Calc-backed probe over the same replay corpus:

- `interpret_tail_probe_formula_cells=50358`
- `interpret_tail_authoritative_total=300`
- `interpret_tail_authoritative_fallback_total=50048`
- raw promoted authoritative rate: `0.61%`

Current promoted-family fallback reasons:

- `shadow_mismatch=50048`
- `unsupported_function=0`
- `unsupported_formula_shape=0`
- `unsupported_host_surface=0`

### Live-Reachable vs Imported-Artifact Promoted Probe

This is the same promoted replay probe, explicitly split into the only two
surfaces that are still interpretable:

- `interpret_tail_probe_live_reachable_formula_cells=300`
- `interpret_tail_probe_imported_artifact_formula_cells=50048`
- `interpret_tail_probe_host_truth_artifact_formula_cells=50048`
- `interpret_tail_live_target_authoritative_total=300`
- `interpret_tail_live_target_authoritative_fallback_total=0`
- live-reachable promoted rate: `0.61%`
- imported-artifact-only promoted rate: `99.39%`

Interpretation:

- the promoted-family denominator is now much broader because scalar roots,
  round-family formulas, information predicates, logical folds, `NOT`, and
  bounded scalar-math formulas are part of the delegated family
- the raw promoted replay probe is now diagnostic, not the deletion
  denominator, and it is only legible once split into live-reachable vs
  imported-artifact-only buckets
- the dominant `shadow_mismatch=50048` residual is overwhelmingly imported
  cached-workbook debt, not live-reachable parity failure
- the raw promoted authoritative rate is now `0.61%`, so this
  surface remains useful for diagnostics but not for retirement steering
- the promoted probe is now best read as `300` live-reachable rows plus
  `50048` imported-artifact rows, not as a single parity percentage
- a focused host-truth test now shows the replay-imported whole-row
  `MATCH([.$B$150];[.$150:.$150];-1)` row evaluates to
  `FormulaError::VariableExpected`
  in live Calc with the seam forced off, so it is not a proven live parity
  blocker even though the replay workbook stores a non-error expected value
- a bounded live-host-truth pass now shows the residual replay-imported
  logical-constant rows are genuine live Calc error rows, not stale cached
  workbook artifacts
- a bounded live-host-truth pass now also shows the replay-imported exact
  `VLOOKUP([.P6];[.$L$2:.$M$8];2;0)` and
  `VLOOKUP(21;[.$AM$2:.$AN$4];2;0)` rows both evaluate to
  `FormulaError::VariableExpected` in Calc with the seam forced off, so those
  cached non-error workbook values are not treated as runtime conversion
  targets
- a bounded live-host-truth pass now also shows the replay-imported
  collation-sensitive exact `VLOOKUP([.M22]; [.L$11:.M$32]; 1; 0)` row
  evaluates to `FormulaError::VariableExpected` in Calc with the seam forced
  off, so the residual replay `VLOOKUP` band is no longer treated as a real
  runtime conversion target
- a bounded live-host-truth pass now also shows the residual replay-imported
  `XLOOKUP("Ireland"; [.H2:.H11]; [.J2:.J11]; "")` and
  `XLOOKUP([.G14]; [.I14:.R14]; [.I15:.R16])` rows both evaluate to
  `FormulaError::VariableExpected` in Calc with the seam forced off, so the
  residual replay `XLOOKUP` band is likewise no longer treated as a real
  runtime conversion target
- a bounded live-host-truth pass now also shows the replay-imported
  `INDEX([.H13:.J19]; XMATCH([.G10]; [.G13:.G19]); XMATCH([.H10]; [.H12:.J12]))`
  row evaluates to `FormulaError::VariableExpected` in Calc with the seam
  forced off, so that cached numeric workbook value is not treated as a real
  runtime conversion target either
- a bounded live-host-truth pass now also shows the replay-imported
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 1)`,
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 2)`, and
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 0)` rows all
  evaluate to `FormulaError::VariableExpected` in Calc with the seam forced
  off, so that imported `INDEX` matrix-function band is likewise no longer
  treated as a real runtime conversion target
- a bounded live-host-truth pass now also shows the replay-imported
  `DATEVALUE("Jan1, 2015")` rows evaluate to `FormulaError::VariableExpected`
  in Calc with the seam forced off, so those cached non-numeric workbook rows
  are not treated as real live runtime conversion targets either
- a bounded live-host-truth pass now also shows the replay-imported
  `MATCH(1; FREQUENCY([.I126]; [.H129:.M129]); 0)` row evaluates to
  `FormulaError::VariableExpected` in Calc with the seam forced off, so that
  cached workbook non-error row is likewise not treated as a real live runtime
  conversion target
- once the probe is filtered by live host truth, there is currently no
  remaining imported promoted replay surface that behaves like a real seam-off
  legacy runtime target
- the raw promoted replay probe is therefore useful as a cached imported
  correctness surface, but not as the live retirement denominator

### Replay Eligibility Inventory

This is the new per-cell replay inventory over the promoted-family replay
surface after forcing each promoted replay formula through direct live
`Interpret()`:

- `interpret_tail_replay_promoted_formula_cells=50358`
- `interpret_tail_replay_promoted_direct_seen=44741`
- `interpret_tail_replay_promoted_direct_supported=44741`
- `interpret_tail_replay_promoted_direct_fallback=0`
- `interpret_tail_replay_promoted_direct_unseen=5607`
- `interpret_tail_replay_promoted_shared_formula_cells=40577`
- `interpret_tail_replay_promoted_shared_top_formula_cells=3105`
- `interpret_tail_replay_promoted_shared_member_formula_cells=37472`
- `interpret_tail_replay_promoted_non_shared_formula_cells=9771`
- `interpret_tail_replay_promoted_unseen_shared_top=258`
- `interpret_tail_replay_promoted_unseen_shared_member=2606`
- `interpret_tail_replay_promoted_unseen_non_shared=2692`
- `interpret_tail_replay_promoted_shared_member_seen_via_top=0`
- `interpret_tail_replay_promoted_needs_interpret_after_dirty=50358`
- `interpret_tail_replay_promoted_dirty_after_interpret=5607`

Interpretation:

- the pre-tail replay eligibility blocker is cleared for the promoted replay
  surface
- the latest slices widened the replay-promoted denominator materially by
  promoting scalar-root, round-family, and math-scalar families on top of the
  earlier text/conversion/statistical widening
- both shared and non-shared promoted replay formulas now reach the live seam
- the direct replay-promoted surface is now fallback-free on the standing
  corpus, but still has a meaningful unseen tail (`5607` cells), especially
  across shared-member and non-shared replay formulas
- the dominant next target is now broader retirement and unseen-surface
  reduction rather than replay fallback cleanup

## Hard-Routed Family

The env-independent engine-first cluster now includes:

- `TRUE()`
- `FALSE()`
- string-literal `VALUE`
- string-literal `DATEVALUE`
- string-literal `TIMEVALUE`
- literal-only `NUMBERVALUE`
- exact `MATCH(<literal>; <1D literal array>; 0)`
- approximate-ascending `MATCH(<literal>; <ascending numeric 1D literal array>; 1)`
- default-approximate `MATCH(<literal>; <ascending numeric 1D literal array>)`
- approximate-descending `MATCH(<literal>; <descending numeric 1D literal array>; -1)`
- default-exact `XMATCH(<literal>; <1D literal array>)`
- exact `XMATCH(<literal>; <1D literal array>; 0)`
- exact-forward `XMATCH(<literal>; <1D literal array>; 0; 1)`
- exact-reverse `XMATCH(<literal>; <1D literal array>; 0; -1)`
- exact-binary-ascending `XMATCH(<literal>; <ascending numeric 1D literal array>; 0; 2)`
- exact-binary-descending `XMATCH(<literal>; <descending numeric 1D literal array>; 0; -2)`
- default-exact-forward `XMATCH(<literal>; <1D literal array> ;; 1)`
- default-exact-reverse `XMATCH(<literal>; <1D literal array> ;; -1)`
- default-exact-binary-ascending `XMATCH(<literal>; <ascending numeric 1D literal array> ;; 2)`
- default-exact-binary-descending `XMATCH(<literal>; <descending numeric 1D literal array> ;; -2)`
- next-larger `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; 1)`
- next-smaller `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; -1)`
- next-larger-forward `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; 1; 1)`
- next-smaller-forward `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; -1; 1)`
- next-larger-reverse `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; 1; -1)`
- next-smaller-reverse `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; -1; -1)`
- next-larger-binary-ascending `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; 1; 2)`
- next-smaller-binary-ascending `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; -1; 2)`
- next-larger-binary-descending `XMATCH(<numeric literal>; <descending numeric 1D literal array>; 1; -2)`
- next-smaller-binary-descending `XMATCH(<numeric literal>; <descending numeric 1D literal array>; -1; -2)`
- `LOOKUP(<literal>; <1D literal vector>)`
- `LOOKUP(<literal>; <1D literal vector>; <1D literal result vector>)`
- `LOOKUP(<literal>; <2D literal matrix>)`
- `VLOOKUP(<literal>; <2D literal array>; <positive whole>; 0)`
- `VLOOKUP(<literal>; <2D literal array>; <positive whole>; FALSE())`
- `VLOOKUP(<literal>; <ascending numeric 2D literal array>; <positive whole>)`
- `VLOOKUP(<literal>; <ascending numeric 2D literal array>; <positive whole>; TRUE())`
- `HLOOKUP(<literal>; <2D literal array>; <positive whole>; 0)`
- `HLOOKUP(<literal>; <2D literal array>; <positive whole>; FALSE())`
- `HLOOKUP(<literal>; <ascending numeric 2D literal array>; <positive whole>)`
- `HLOOKUP(<literal>; <ascending numeric 2D literal array>; <positive whole>; TRUE())`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>)`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found>)`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;; 0)`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found>; 0)`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;; 0; 1)`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found>; 0; 1)`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;; 0; -1)`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found>; 0; -1)`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;;; 1)`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found> ;; 1)`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;;; -1)`
- `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found> ;; -1)`
- `XLOOKUP(<literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; 0; 2)`
- `XLOOKUP(<literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 0; 2)`
- `XLOOKUP(<literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;;; 2)`
- `XLOOKUP(<literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found> ;; 2)`
- `XLOOKUP(<literal>; <descending numeric 1D literal array>; <1D literal result vector> ;; 0; -2)`
- `XLOOKUP(<literal>; <descending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 0; -2)`
- `XLOOKUP(<literal>; <descending numeric 1D literal array>; <1D literal result vector> ;;; -2)`
- `XLOOKUP(<literal>; <descending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found> ;; -2)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; 1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; -1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; -1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; 1; 1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 1; 1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; -1; 1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; -1; 1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; 1; -1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 1; -1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; -1; -1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; -1; -1)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; 1; 2)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 1; 2)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; -1; 2)`
- `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; -1; 2)`
- `XLOOKUP(<numeric literal>; <descending numeric 1D literal array>; <1D literal result vector> ;; 1; -2)`
- `XLOOKUP(<numeric literal>; <descending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 1; -2)`
- `XLOOKUP(<numeric literal>; <descending numeric 1D literal array>; <1D literal result vector> ;; -1; -2)`
- `XLOOKUP(<numeric literal>; <descending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; -1; -2)`
- `INDEX(<2D literal array>; <positive whole>)`
- `INDEX(<2D literal array>; <positive whole>; <positive whole>)`
- `INDEX(<2D literal array>; 0; <positive whole>)`
- `INDEX(<2D literal array>; <positive whole>; 0)`

Current meaning:

- the formula is classified and attempted through the engine even when the
  rollout env var is explicitly `off`
- supported results bypass the normal rollout gate and are applied
  authoritatively
- unsupported or projection-failure cases still fall back safely
- `TRUE()` / `FALSE()` now also have an explicit family-local default-on
  rollout path, and their dedicated `ScInterpreter` subroutines are deleted
- the dedicated `ScInterpreter` wrapper pair for string-literal `DATEVALUE` /
  `TIMEVALUE` is also deleted; nested legacy evaluation stays inline at
  dispatch while the existing env-`off` engine-first root slice remains intact
- `ScInterpreter::ScValue()`, `ScInterpreter::ScNumberValue()`, and
  `ScInterpreter::ScMatch()`, `ScInterpreter::ScXMatch()`,
  `ScInterpreter::ScLookup()`, `ScInterpreter::ScVLookup()`,
  `ScInterpreter::ScHLookup()`, `ScInterpreter::ScXLookup()`, and
  `ScInterpreter::ScIndex()` now treat those narrow slices as quarantined
  legacy paths and emit a debug warning if normal interpreter execution
  reaches them

That earlier hard-route milestone completed the adjacent
default/approximate `MATCH` plus omitted/approximate extended-match
quarantine frontier. Since then, part of that older surface has moved into
family-local default-on rollout, leaving `83` env-independent hard-routed
slices plus the newer default-on families.

## Scope Policy

The active roadmap is now evaluator migration, not broad substrate widening.

New computational-substrate widening work is out of scope unless it does one
of these things:

- removes an `InterpretTail` live fallback reason
- removes an `InterpretTail` shadow mismatch class
- unlocks required host access for a promoted evaluator family

Residual substrate frontier items that do not satisfy one of those bars are
historical reference material, not active roadmap work.

## Current Next Target

The latest replay closeout is now folded into this migration ledger, but the
next target is no longer "the next 62 lambdas" as if they were a flat
function queue.

The remaining work is now a single subsystem initiative:

- build an engine-native `RpnEvaluator`
- shrink `interp4_dispatch_legacy_lambda_count` through genuine engine-side
  operator, control-flow, reference, and matrix execution
- use wrapper deletion only as a companion metric, not the lead story

The planning documents for that phase are now:

- [COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md](COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md)
- [COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md](COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md)

## Current Next Pass

The current next pass is:

1. complete the host-boundary audit
2. define the minimal host contract for full engine-side RPN evaluation
3. build the engine stack-value model and typed coercion layer
4. move the operator opcode block into the engine
5. then move control-flow opcodes into the engine loop

Guardrails:

- keep `unsupported_function=0`
- keep `fallback=0`
- prefer shrinking `interp4_dispatch_legacy_lambda_count` over shrinking
  `legacy_interpreter_subroutine_count` if only one can move honestly in the
  short term

## Historical Archive

The retired pass-by-pass ledger lives in:

- [../archive/interpret_tail/](../archive/interpret_tail/)
