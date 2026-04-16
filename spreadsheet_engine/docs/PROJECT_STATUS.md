# Spreadsheet Engine: Project Status

This file is the concise current-state snapshot for `spreadsheet_engine/`.

Start here for the active migration story:

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)

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
- `interpret_tail_live_authoritative_probe_formula_cells=27204`
- `interpret_tail_live_authoritative_match_total=20761`
- `interpret_tail_live_authoritative_fallback_total=6443`
- `legacy_interpreter_subroutine_count=380`
- live authoritative-match rate over the corpus: `40.9802%`
- live authoritative-match rate over the current promoted probe: `76.3160%`

The north-star measures live authority transfer. `legacy_interpreter_subroutine_count`
is the blunt retirement-progress companion metric, derived from the remaining
`void Sc*()` declarations in [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx).
Lower is better.

Everything below is diagnostic context for improving that number.

### Full Replay Corpus: Ambient Live Observe Attempts

These are attempt totals from live observe, not unique-cell coverage. They
remain useful for hotspot steering, but they should not be read as the
deletion denominator.

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=92486`
- `interpret_tail_live_fallback_total=294`
- `interpret_tail_live_seen_total=92780`
- `interpret_tail_live_unseen_formula_cells=0`
- `interpret_tail_live_promoted_function_supported_total=91884`
- `interpret_tail_live_supported_rate=182.56`
- `interpret_tail_live_seen_rate=183.14`

Ambient live fallback reasons:

- `unsupported_formula_shape=122`
- `unsupported_host_surface=0`
- `parse_failure=4`
- `unsupported_function=168`

### Full Replay Corpus: Live Unique-Cell Surface

This is the honest per-formula-cell live-routing surface from the standing
replay corpus. It counts whether each formula cell was actually seen and
supported during the bulk live observe run.

- `interpret_tail_live_unique_formula_cells=50661`
- `interpret_tail_live_unique_seen_formula_cells=27571`
- `interpret_tail_live_unique_supported_formula_cells=27435`
- `interpret_tail_live_unique_fallback_formula_cells=136`
- `interpret_tail_live_unique_unsupported_function_formula_cells=84`
- `interpret_tail_live_unique_unseen_formula_cells=23090`
- `interpret_tail_live_unique_seen_rate=54.42`
- `interpret_tail_live_unique_supported_rate=54.15`

### Live Unique Unsupported-Function Top-N

This is now the routing table for the next ambient `unsupported_function`
work. It is unique-cell inventory, not attempt telemetry.

- `interpret_tail_live_unique_unsupported_function_formula_cells=84`
- `unknown`: `54` unique unsupported-function cells
- `text_utility`: `19` unique unsupported-function cells
- `conditional`: `11` unique unsupported-function cells

Next routing policy:

- first keep burning down the now-smaller `unknown` bucket by taking the
  highest-count bounded roots directly
- then burn down `text_utility`
- then burn down `conditional`

### Unknown Bucket Root Split

The top live unique roots still inside `FunctionKind::Unknown` are now:

- `ORG.LIBREOFFICE.FORECAST.ETS.MULT`: `3` unique unsupported-function cells
- `COM.MICROSOFT.FORECAST.ETS`: `3`
- `COM.MICROSOFT.MODE.MULT`: `3`
- `COM.MICROSOFT.VSTACK`: `3`
- `COM.MICROSOFT.MODE.SNGL`: `2`
- `COMPLEX`: `2`
- `KURT`: `2`
- `MODE`: `2`
- `COM.MICROSOFT.COVARIANCE.P`: `2`
- `COM.MICROSOFT.COVARIANCE.S`: `2`

Updated next routing policy:

- the bounded selector cluster is now admitted and tracked as
  `selector=62` live unique cells, `58` supported / `4` fallback
- the new `matrix_math` default-on slice now carries `MDETERM` as
  `12` live unique cells, `10` supported / `2` fallback, and the dedicated
  `ScInterpreter::ScMatDet()` wrapper is retired
- the bounded `AGGREGATE` slice is now also a default-on retirement slice,
  and the dedicated `ScInterpreter::ScAggregate()` wrapper is retired
- the narrow `PROB(...)` slice is now default-on as well, and the dedicated
  `ScInterpreter::ScProbability()` wrapper is retired
- the new bounded `growth_projection` slice now carries `GROWTH` as
  `10` live unique cells, `10` supported / `0` fallback, removing it from the
  `unknown` wall
- the scalar `TEXTAFTER(...)` slice is now admitted through `text_utility`
- `TEXTBEFORE(...)` is now admitted through `text_utility`, with focused
  imported parity proof
- the `IFS(...)` / `SWITCH(...)` scalar slice is now admitted through
  `conditional`
- the spill-heavy dynamic-array cluster
  (`UNIQUE`, `SORT`, `SORTBY`, `TEXTSPLIT`, `HSTACK`) is now admitted as
  `spill_array=83` live unique cells, `73` supported / `10` fallback, with
  `unsupported_function=0` and the residual concentrated in
  `unsupported_formula_shape=10`
- the broad corpus lane is stable again after fixing the intermittent
  `CONVERT(...)` runtime crash in the shared BFS conversion path
- the bounded `FORECAST(...)` / `INTERCEPT(...)` regression slice is now
  admitted through `statistical_distribution`, removing both roots from the
  live unique `unknown` wall
- next routing should now move off the spill cluster and onto the remaining
  bounded forecasting/statistical unknown roots, starting with
  `MODE.MULT` / `MODE.SNGL`, `KURT`, and adjacent covariance roots before
  heavier ETS or spill-shaped work

### Full Replay Corpus: Forced Interpret Observe Attempts

These are also attempt totals. The new unique-cell direct surface is the
honest coverage metric below.

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=1681808`
- `interpret_tail_forced_interpret_fallback_total=147`
- `interpret_tail_forced_interpret_seen_total=1681955`
- `interpret_tail_forced_interpret_unseen_formula_cells=0`
- `interpret_tail_forced_interpret_promoted_function_supported_total=1681507`
- `interpret_tail_forced_interpret_supported_rate=3319.73`
- `interpret_tail_forced_interpret_seen_rate=3320.02`

### Full Replay Corpus: Forced Direct Unique-Cell Surface

This is the direct-routing comparison surface after dirtying and forcing each
replay formula cell once, then classifying whether that formula cell was
actually seen and supported by the seam.

- `interpret_tail_forced_direct_formula_cells=50661`
- `interpret_tail_forced_direct_seen_formula_cells=27515`
- `interpret_tail_forced_direct_supported_formula_cells=27379`
- `interpret_tail_forced_direct_fallback_formula_cells=136`
- `interpret_tail_forced_direct_unseen_formula_cells=23146`
- `interpret_tail_forced_direct_seen_rate=54.31`
- `interpret_tail_forced_direct_supported_rate=54.04`

### Raw Cached-Workbook Promoted Probe

- `interpret_tail_probe_formula_cells=27204`
- `interpret_tail_authoritative_total=5587`
- `interpret_tail_authoritative_fallback_total=21617`
- raw promoted authoritative rate: `20.54%`

Raw promoted fallback reasons:

- `shadow_mismatch=21547`
- `unsupported_function=30`
- `unsupported_formula_shape=40`
- `unsupported_host_surface=0`

### Live-Reachable Promoted Probe

- `interpret_tail_probe_live_reachable_formula_cells=0`
- `interpret_tail_live_target_authoritative_total=0`
- `interpret_tail_live_target_authoritative_fallback_total=0`
- live-reachable promoted rate: `0.00%`

### Imported-Artifact-Only Promoted Probe

- `interpret_tail_probe_imported_artifact_formula_cells=27204`
- `interpret_tail_probe_host_truth_artifact_formula_cells=27204`
- imported-artifact-only promoted rate: `100.00%`

Interpretation:

- the raw promoted probe is now overwhelmingly imported cached-workbook debt,
  not a live parity denominator
- the promoted probe is only interpretable when split into live-reachable vs
  imported-artifact-only buckets
- the `shadow_mismatch=21547` wall is real diagnostic debt, but it is almost
  entirely on imported-artifact-only rows rather than live-reachable parity rows

### Promoted Replay Eligibility Inventory

- `interpret_tail_replay_promoted_formula_cells=27204`
- `interpret_tail_replay_promoted_direct_seen=27148`
- `interpret_tail_replay_promoted_direct_supported=27078`
- `interpret_tail_replay_promoted_direct_fallback=70`
- `interpret_tail_replay_promoted_direct_unseen=56`
- `interpret_tail_replay_promoted_shared_formula_cells=21779`
- `interpret_tail_replay_promoted_non_shared_formula_cells=5425`
- `interpret_tail_replay_promoted_unseen_shared_member=27`
- `interpret_tail_replay_promoted_unseen_non_shared=24`
- `interpret_tail_replay_promoted_shared_member_seen_via_top=8`
- `interpret_tail_replay_promoted_needs_interpret_after_dirty=27204`
- `interpret_tail_replay_promoted_dirty_after_interpret=56`

### Engine-Authoritative Families

Default-on families:

- logical constants: `TRUE()` / `FALSE()`
- formula text: `FORMULA(...)`
- conversion family: `CONVERT(...)`, `ORG.OPENOFFICE.CONVERT(...)`,
  `EUROCONVERT(...)`, `BASE(...)`, `DECIMAL(...)`, `ROMAN(...)`,
  `ARABIC(...)`
- significant rounding: `ROUNDSIG(...)`, `ORG.LIBREOFFICE.ROUNDSIG(...)`
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
  quarantine cluster, while logical constants, formula text, conversion,
  significant rounding, bitwise, aggregate, matrix determinant, and `PROB`
  now also have family-local default-on rollout paths
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

- no broad default-on rollout exists beyond the narrow logical-constant,
  formula-text, and conversion families plus the narrow `ROUNDSIG`,
  bitwise, aggregate, matrix-determinant, and `PROB` slices
- eleven narrow legacy deletion milestones have landed:
  `ScInterpreter::ScTrue()` / `ScFalse()` and the dedicated
  `ScGetDateValue()` / `ScGetTimeValue()` wrapper pair, plus the dedicated
  `ScInterpreter::ScFormula()` wrapper, and now the dedicated
  `ScInterpreter::ScConvertOOo()` wrapper, and now the dedicated
  `ScInterpreter::ScRoundSignificant()` wrapper, and now the dedicated
  `ScInterpreter::ScEuroConvert()` wrapper, and now the dedicated
  `ScInterpreter::ScBase()` / `ScDecimal()` / `ScRoman()` / `ScArabic()`
  wrappers, and now the dedicated `ScInterpreter::ScBitAnd()` /
  `ScBitOr()` / `ScBitXor()` / `ScBitLshift()` / `ScBitRshift()`
  wrappers, and now the dedicated `ScInterpreter::ScMatDet()` wrapper,
  and now the dedicated `ScInterpreter::ScAggregate()` wrapper, and now the
  dedicated `ScInterpreter::ScProbability()` wrapper; broader legacy
  retirement has not started
- multiple interpreter hard-route milestones have landed, but full legacy
  opcode retirement has not
- the dominant retained live blocker is now the broader ambient
  `unsupported_function=168` wall rather than residual quality inside the
  already-admitted families
- the live authoritative-match north-star has now moved to
  `20761 / 50,661` (`40.9802%`) on the replay corpus
- that gain now includes imported direct information-predicate host-truth
  parity on unsupported expression roots, plus the follow-on imported
  direct logical-fold and nested-`XMATCH` `INDEX` host-truth cleanup, and the
  new bounded ranked-statistical plus calendar/date utility clusters, and a
  default-on `BusinessDay` family that now rejects zero-workday `WORKDAY`
  weekend masks before entering the shared runtime, plus newly admitted local
  reference, holiday-range, weekend-range, weekend-code-ref, and named-ref
  `BusinessDay` shapes, and now a bounded criteria-aggregate slice for
  `COUNTIF` / `COUNTIFS`, `SUMIF` / `SUMIFS`, `AVERAGEIF` / `AVERAGEIFS`, and
  `MAXIFS` / `MINIFS`, with the remaining matrix-`IF(...)` criteria residue
  now closed on the live unique-cell surface, and a new bounded scalar
  `IF(...)` family that contributes `1887` live unique seen cells with
  `1873` supported and `14` fallback, followed now by a bounded
  `text_utility` family contributing `881` live unique seen cells with
  `859` supported and `22` fallback, and now a bounded
  `statistical_distribution` family contributing `1039` live unique seen
  cells with `1037` supported and `2` fallback, and now imported
  `FORMULA(...)` live-host-truth alignment that contributes
  `14278` live unique seen cells with `14278` supported and `0` fallback,
  followed now by a bounded `CONVERT` slice that contributes
  `308` live unique seen cells with `308` supported and `0` fallback, with
  the same conversion family now also carrying default-on numeral-conversion
  `BASE` / `DECIMAL` / `ROMAN` / `ARABIC` retirement, plus a bounded
  `AGGREGATE` slice that contributes `212` live unique seen cells with
  `211` supported and `1` fallback, plus a bounded `matrix_math` slice that
  contributes `12` live unique seen cells with `10` supported and `2`
  fallback, and a widened `statistical_distribution` family that now sits at
  `1052` live unique seen cells with `1049` supported and `3` fallback after
  admitting `PROB(...)`, while the same retirement pass also promotes
  `AGGREGATE`, `MDETERM`, and `PROB` to family-local default-on ownership and
  deletes their dedicated Calc wrappers, followed now by a bounded
  `growth_projection` slice contributing `10` live unique seen cells with
  `10` supported and `0` fallback, which removes `GROWTH` from the live
  unique `unknown` wall
- the raw promoted replay probe is now unambiguously a diagnostic surface, not
  the retirement denominator: it now sits at `5587 / 27204`, dominated by
  imported `FORMULA(...)` cached-workbook string mismatches even while the
  live host truth for those rows is now matched
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
  parity and live-target filtered parity, and the filtered surface is empty:
  all `27008` promoted replay probe rows are imported host-truth artifacts
  under seam-off direct legacy interpretation
- the dominant retained live bucket is now the still-large ambient
  `unsupported_function=168`; the raw promoted buckets remain diagnostic debt,
  not the deletion-gating story
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
  `GCD`, `LCM`, and related aliases
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

The next pass is now constrained by the scope gate:

1. do not add new hard-route slices unless they remove a live fallback reason
   or a live mismatch bucket
2. treat live authoritative-match as the single north-star metric for
   retirement progress
3. treat the raw promoted replay probe as a cached imported correctness
   surface, not as the live retirement denominator
4. keep targeting slices that increase live authoritative-match directly,
   starting with the remaining ambient `unsupported_function` wall rather
   than reopening replay-probe cleanup
5. keep `NETWORKDAYS`, `WORKDAY`, `NETWORKDAYS.INTL`, and `WORKDAY.INTL`
   behind explicit opt-in until the ambient evaluator path is cheap enough for
   broad corpus measurement
6. keep attacking the remaining ambient `unsupported_function` wall with cheap,
   measurable clusters after calendar/date utility admission
7. use the logical-constant and date/time wrapper deletion milestones as the
   template for the next narrow retirement only after that ambient wall pays
   down further
8. only return to imported replay parity if we intentionally decide to
   rehabilitate legacy seam-off imported-formula execution

## References

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md)
- [archive/interpret_tail/](archive/interpret_tail/)
- [archive/pre_pivot_substrate/](archive/pre_pivot_substrate/)
