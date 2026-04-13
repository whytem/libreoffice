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

### Full Replay Corpus: Ambient Live Observe

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=4326`
- `interpret_tail_live_fallback_total=622`
- `interpret_tail_live_seen_total=4948`
- `interpret_tail_live_unseen_formula_cells=45713`
- `interpret_tail_live_promoted_function_supported_total=3076`
- `interpret_tail_live_supported_rate=8.54`
- `interpret_tail_live_seen_rate=9.77`

Ambient live fallback reasons:

- `unsupported_formula_shape=32`
- `unsupported_host_surface=0`
- `parse_failure=4`
- `unsupported_function=588`

### Full Replay Corpus: Forced Interpret Observe

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=2163`
- `interpret_tail_forced_interpret_fallback_total=311`
- `interpret_tail_forced_interpret_seen_total=2474`
- `interpret_tail_forced_interpret_unseen_formula_cells=48187`
- `interpret_tail_forced_interpret_promoted_function_supported_total=1538`
- `interpret_tail_forced_interpret_supported_rate=4.27`
- `interpret_tail_forced_interpret_seen_rate=4.88`

### Promoted-Family Probe

- `interpret_tail_probe_formula_cells=1812`
- `interpret_tail_authoritative_total=1794`
- `interpret_tail_authoritative_fallback_total=18`
- promoted-family authoritative rate: `99.01%`

### Live-Target Filtered Promoted Probe

- `interpret_tail_live_target_probe_formula_cells=0`
- `interpret_tail_probe_host_truth_artifact_formula_cells=1812`
- `interpret_tail_live_target_authoritative_total=0`
- `interpret_tail_live_target_authoritative_fallback_total=0`

Promoted-family fallback reasons:

- `unsupported_formula_shape=5`
- `shadow_mismatch=13`
- `unsupported_host_surface=0`

### Promoted Replay Eligibility Inventory

- `interpret_tail_replay_promoted_formula_cells=1812`
- `interpret_tail_replay_promoted_direct_seen=1812`
- `interpret_tail_replay_promoted_direct_supported=1806`
- `interpret_tail_replay_promoted_direct_fallback=6`
- `interpret_tail_replay_promoted_direct_unseen=0`
- `interpret_tail_replay_promoted_shared_formula_cells=395`
- `interpret_tail_replay_promoted_non_shared_formula_cells=1417`
- `interpret_tail_replay_promoted_unseen_shared_member=0`
- `interpret_tail_replay_promoted_unseen_non_shared=0`
- `interpret_tail_replay_promoted_shared_member_seen_via_top=0`
- `interpret_tail_replay_promoted_needs_interpret_after_dirty=1812`
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
- corresponding legacy entrypoints now carry debug quarantine warnings on
  normal interpreter reach:
  - `ScInterpreter::ScTrue()`
  - `ScInterpreter::ScFalse()`
  - `ScInterpreter::ScValue()`
  - `ScInterpreter::ScGetDateValue()`
  - `ScInterpreter::ScGetTimeValue()`
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
- the latest bounded default/approximate `MATCH` plus omitted/approximate
  extended-match slice raised the env-independent hard-route cluster from
  `50` to `85` without opening a new delegated family
- within the current families, the semantically distinct env-independent
  literal-array hard-route surface is now effectively exhausted; further
  widening would mainly mean alias recounts or pattern/collation-sensitive
  modes

Still not true:

- no broad default-on rollout exists
- no `ScInterpreter` subroutine has been deleted yet
- multiple interpreter hard-route milestones have landed, but full legacy
  opcode retirement has not
- full replay-corpus live traffic is now material, but most supported ambient
  traffic still covers only a bounded minority of formulas
- the replay-promoted reach blocker is cleared, and the latest slice reduced
  replay-imported promoted fallback further
- the dominant retained promoted-family blocker is still `shadow_mismatch`,
  with smaller `unsupported_formula_shape` cleanup secondary
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
  all `1812` promoted replay probe rows are imported host-truth artifacts under
  seam-off direct legacy interpretation
- the next runtime milestone therefore should not be defined by the imported
  replay probe anymore; it should move to broader ambient live reach,
  additional Calc-path quarantine / retirement slices, or deliberate
  function-family expansion

## Active Delegated Family

The live delegated evaluator family currently includes:

- `TRUE`
- `FALSE`
- `VALUE`
- `DATEVALUE`
- `TIMEVALUE`
- `NUMBERVALUE`
- `MATCH`
- `XMATCH`
- `LOOKUP`
- `VLOOKUP`
- `HLOOKUP`
- `XLOOKUP`
- `INDEX`
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

The next pass should now move off imported replay parity cleanup:

1. treat the raw promoted replay probe as a cached imported correctness surface,
   not as the live retirement denominator
2. drive the next real runtime win on broader ambient live reach or another
   narrow hard-route / quarantine slice inside `ScInterpreter`
3. only return to imported replay parity if we intentionally decide to
   rehabilitate legacy seam-off imported-formula execution

## References

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md)
- [archive/interpret_tail/](archive/interpret_tail/)
- [archive/pre_pivot_substrate/](archive/pre_pivot_substrate/)
