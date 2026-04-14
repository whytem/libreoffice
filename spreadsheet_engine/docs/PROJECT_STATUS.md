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
- `interpret_tail_live_authoritative_probe_formula_cells=7063`
- `interpret_tail_live_authoritative_match_total=4389`
- `interpret_tail_live_authoritative_fallback_total=2674`
- live authoritative-match rate over the corpus: `8.6635%`
- live authoritative-match rate over the current promoted probe: `62.1407%`

Everything below is diagnostic context for improving that number.

### Full Replay Corpus: Ambient Live Observe

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=19198`
- `interpret_tail_live_fallback_total=604`
- `interpret_tail_live_seen_total=19802`
- `interpret_tail_live_unseen_formula_cells=30859`
- `interpret_tail_live_promoted_function_supported_total=17948`
- `interpret_tail_live_supported_rate=37.90`
- `interpret_tail_live_seen_rate=39.09`

Ambient live fallback reasons:

- `unsupported_formula_shape=22`
- `unsupported_host_surface=0`
- `parse_failure=4`
- `unsupported_function=586`

### Full Replay Corpus: Forced Interpret Observe

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=47193`
- `interpret_tail_forced_interpret_fallback_total=302`
- `interpret_tail_forced_interpret_seen_total=47495`
- `interpret_tail_forced_interpret_unseen_formula_cells=3166`
- `interpret_tail_forced_interpret_promoted_function_supported_total=46566`
- `interpret_tail_forced_interpret_supported_rate=93.15`
- `interpret_tail_forced_interpret_seen_rate=93.75`

### Promoted-Family Probe

- `interpret_tail_probe_formula_cells=7063`
- `interpret_tail_authoritative_total=2628`
- `interpret_tail_authoritative_fallback_total=4435`
- promoted-family authoritative rate: `37.21%`

### Live-Target Filtered Promoted Probe

- `interpret_tail_live_target_probe_formula_cells=0`
- `interpret_tail_probe_host_truth_artifact_formula_cells=7063`
- `interpret_tail_live_target_authoritative_total=0`
- `interpret_tail_live_target_authoritative_fallback_total=0`

Promoted-family fallback reasons:

- `shadow_mismatch=2815`
- `unsupported_function=14`
- `unsupported_formula_shape=10`
- `unsupported_host_surface=6`

### Promoted Replay Eligibility Inventory

- `interpret_tail_replay_promoted_formula_cells=7063`
- `interpret_tail_replay_promoted_direct_seen=7063`
- `interpret_tail_replay_promoted_direct_supported=7033`
- `interpret_tail_replay_promoted_direct_fallback=30`
- `interpret_tail_replay_promoted_direct_unseen=0`
- `interpret_tail_replay_promoted_shared_formula_cells=3331`
- `interpret_tail_replay_promoted_non_shared_formula_cells=3732`
- `interpret_tail_replay_promoted_unseen_shared_member=0`
- `interpret_tail_replay_promoted_unseen_non_shared=0`
- `interpret_tail_replay_promoted_shared_member_seen_via_top=0`
- `interpret_tail_replay_promoted_needs_interpret_after_dirty=7063`
- `interpret_tail_replay_promoted_dirty_after_interpret=0`

### Current Hard-Routed Family Count

- `85` env-independent engine-first slices:
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
- `TRUE()` / `FALSE()` now also have an explicit family-local default-on
  rollout path, and their dedicated `ScInterpreter` subroutines are deleted
- the dedicated `ScInterpreter` wrapper pair for string-literal `DATEVALUE` /
  `TIMEVALUE` is also deleted; nested legacy evaluation stays inline at
  dispatch while the existing env-`off` engine-first root slice remains intact
- corresponding legacy entrypoints now carry debug quarantine warnings on
  normal interpreter reach:
  - `ScInterpreter::ScValue()`
  - `ScInterpreter::ScNumberValue()`
  - `ScInterpreter::ScMatch()`
  - `ScInterpreter::ScXMatch()`
  - `ScInterpreter::ScLookup()`
  - `ScInterpreter::ScVLookup()`
  - `ScInterpreter::ScHLookup()`
  - `ScInterpreter::ScXLookup()`
  - `ScInterpreter::ScIndex()`

## Current State

Today:

- the shared compiler, token model, workbook model, FODS loader, evaluator,
  dependency snapshot, invalidation planning, and recalc planning are
  engine-owned
- the `InterpretTail` seam is real production code, not a test-only oracle
- `DBG_UTIL` builds default to `observe` when the rollout env var is unset
- `authority` mode authoritatively bypasses `ScInterpreter` for supported
  promoted families
- the env-`off` hard-route boundary now covers an eighty-five-slice
  logical/text/match/xmatch/lookup/index cluster instead of just
  `NUMBERVALUE`
- the full replay corpus now has a true all-formula live-routing denominator
- replay-imported promoted formulas now reach the seam broadly, and bounded
  top-level `INDEX` / `XLOOKUP` slice results now stay inside it
- the latest deliberate underlying math-feeder expansion now delegates a
  bounded scalar-math family beneath comparison-helper ranges
- the latest bounded default/approximate `MATCH` plus omitted/approximate
  extended-match slice raised the env-independent hard-route cluster from
  `50` to `85` without opening a new delegated family
- within the current families, the semantically distinct env-independent
  literal-array hard-route surface is now effectively exhausted
- new hard-route widening is now frozen unless it removes a live fallback
  reason or a live mismatch bucket

Still not true:

- no broad default-on rollout exists beyond logical constants
- two narrow legacy deletion milestones have landed:
  `ScInterpreter::ScTrue()` / `ScFalse()` and the dedicated
  `ScGetDateValue()` / `ScGetTimeValue()` wrapper pair; broader legacy
  retirement has not started
- multiple interpreter hard-route milestones have landed, but full legacy
  opcode retirement has not
- the dominant retained live blocker is now quality inside that newly admitted
  traffic, especially logical-fold shadow mismatches and math-scalar shadow
  mismatches, not simple lack of ambient reach
- the live authoritative-match north-star has now moved to
  `4389 / 50,661` (`8.6635%`) on the replay corpus
- that gain now includes imported direct information-predicate host-truth
  parity on unsupported expression roots, plus the follow-on imported
  direct logical-fold and nested-`XMATCH` `INDEX` host-truth cleanup
- the raw promoted replay probe is now unambiguously a diagnostic surface, not
  the retirement denominator: it intentionally sits at `2628 / 4435`
  because live-host parity now takes precedence over cached-workbook parity on
  those imported rows
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
  all `7063` promoted replay probe rows are imported host-truth artifacts under
  seam-off direct legacy interpretation
- the dominant retained live buckets are now the still-large ambient
  `unsupported_function=586`; the raw promoted buckets remain diagnostic debt,
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
   starting with the remaining imported live-host-truth buckets that still
   sit inside `information_predicate`, residual `logical_constant`, and
   smaller lookup / structural residue
5. use the logical-constant and date/time wrapper deletion milestones as the
   template for the next narrow retirement only after `math_scalar` /
   `logical_fold` mismatch reduction pays down more live fallback
6. only return to imported replay parity if we intentionally decide to
   rehabilitate legacy seam-off imported-formula execution

## References

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md)
- [archive/interpret_tail/](archive/interpret_tail/)
- [archive/pre_pivot_substrate/](archive/pre_pivot_substrate/)
