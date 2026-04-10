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
- one tiny family is now engine-first even with rollout set to `off`:
  literal-only `NUMBERVALUE`

What is still not true:

- no broad default-on production rollout exists
- no `ScInterpreter` subroutine has been deleted yet
- the full replay corpus now shows material ambient live-seam traffic
- most of that new ambient supported traffic is still root-error or bounded
  logical-literal traffic, not yet promoted-family function traffic
- even the new full replay forced-interpret denominator still does not surface
  promoted families live, and the new replay eligibility inventory shows the
  dominant blocker is a non-shared pre-tail replay path rather than
  shared-group entry

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

### Full Replay Corpus: Ambient Live Observe

This is the honest all-formula live-routing surface from the standing replay
corpus after the completed ambient residual-blocker conversion slice:

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=606`
- `interpret_tail_live_fallback_total=0`
- `interpret_tail_live_seen_total=606`
- `interpret_tail_live_unseen_formula_cells=50055`
- `interpret_tail_live_promoted_function_supported_total=0`
- `interpret_tail_live_supported_rate=1.20`
- `interpret_tail_live_seen_rate=1.20`

Current ambient fallback reasons:

- `unsupported_formula_shape=0`
- `unsupported_function=0`
- `parse_failure=0`

Interpretation:

- the seam now measures the full replay corpus
- the ambient live-routing denominator is real, not curated
- the full replay corpus now reaches the seam materially and clears with zero
  retained ambient fallback
- the newly supported ambient traffic is still mostly root-error traffic plus
  bounded logical-literal traffic
- the ambient replay surface still does not include promoted-family function
  traffic

### Full Replay Corpus: Forced Interpret Observe

This is the new full-corpus measurement after explicitly dirtying and forcing
every replay formula cell through Calc's live `Interpret()` path:

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=303`
- `interpret_tail_forced_interpret_fallback_total=0`
- `interpret_tail_forced_interpret_seen_total=303`
- `interpret_tail_forced_interpret_unseen_formula_cells=50358`
- `interpret_tail_forced_interpret_promoted_function_supported_total=0`
- `interpret_tail_forced_interpret_supported_rate=0.60`
- `interpret_tail_forced_interpret_seen_rate=0.60`

Interpretation:

- the replay barrier is not just incidental getter access
- even direct live interpretation on the replay corpus still fails to surface
  promoted families
- the next value is identifying the pre-tail live eligibility barrier, not
  widening the evaluator family again

### Promoted-Family Probe

This is the promoted-family Calc-backed probe over the same replay corpus:

- `interpret_tail_probe_formula_cells=1812`
- `interpret_tail_authoritative_total=1698`
- `interpret_tail_authoritative_fallback_total=114`
- promoted-family authoritative rate: `93.71%`

Current promoted-family fallback reasons:

- `unsupported_formula_shape=26`
- `shadow_mismatch=69`
- `unsupported_host_surface=19`

Interpretation:

- once the probe hits promoted-family cells, authority conversion is now
  strong
- the main remaining conversion work is no longer generic breadth
- the current hotspots are residual mismatch and host-surface cleanup on the
  already-promoted families

### Replay Eligibility Inventory

This is the new per-cell replay inventory over the promoted-family replay
surface after forcing each promoted replay formula through direct live
`Interpret()`:

- `interpret_tail_replay_promoted_formula_cells=1812`
- `interpret_tail_replay_promoted_direct_seen=2`
- `interpret_tail_replay_promoted_direct_unseen=1810`
- `interpret_tail_replay_promoted_shared_formula_cells=395`
- `interpret_tail_replay_promoted_shared_top_formula_cells=83`
- `interpret_tail_replay_promoted_shared_member_formula_cells=312`
- `interpret_tail_replay_promoted_non_shared_formula_cells=1417`
- `interpret_tail_replay_promoted_unseen_shared_top=83`
- `interpret_tail_replay_promoted_unseen_shared_member=312`
- `interpret_tail_replay_promoted_unseen_non_shared=1415`
- `interpret_tail_replay_promoted_shared_member_seen_via_top=0`
- `interpret_tail_replay_promoted_needs_interpret_after_dirty=1812`
- `interpret_tail_replay_promoted_dirty_after_interpret=0`

Interpretation:

- the blocker is not “cell never becomes dirty enough to interpret”
- the blocker is not primarily shared-group top entry
- the dominant missed surface is non-shared promoted replay formulas
- the next migration value is now instrumenting or converting the replay path
  between `Interpret()` entry and `InterpretTail` reach for non-shared imported
  formulas

## Hard-Routed Family

The first env-independent engine-first family is now live:

- literal-only `NUMBERVALUE`

Current meaning:

- the formula is classified and attempted through the engine even when the
  rollout env var is explicitly `off`
- supported results bypass the normal rollout gate and are applied
  authoritatively
- unsupported or projection-failure cases still fall back safely

This is the first hard-quarantined Calc path on the migration track.

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

The latest replay closeout is now folded into this migration ledger.

The next high-value pass should now focus on the non-shared replay barrier:

1. instrument the pre-tail replay path for promoted non-shared formulas between
   `Interpret()` entry and `InterpretTail` reach
2. compare replay-imported promoted formulas against curated probe formulas at
   the token or code-path level
3. target the dominant non-shared replay families first:
   `LOOKUP`, `VLOOKUP`, and promoted logical constants
4. only return to shared-group replay work if the non-shared barrier stops
   dominating

## Historical Archive

The retired pass-by-pass ledger lives in:

- [../archive/interpret_tail/](../archive/interpret_tail/)
