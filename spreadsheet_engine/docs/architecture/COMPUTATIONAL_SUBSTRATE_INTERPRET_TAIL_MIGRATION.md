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
- the full replay corpus still shows very little ambient live-seam traffic

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
corpus:

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=0`
- `interpret_tail_live_fallback_total=303`
- `interpret_tail_live_seen_total=303`
- `interpret_tail_live_unseen_formula_cells=50358`
- `interpret_tail_live_supported_rate=0.00`
- `interpret_tail_live_seen_rate=0.60`

Current ambient fallback reasons:

- `unsupported_formula_shape=253`
- `parse_failure=48`
- `unsupported_function=2`

Interpretation:

- the seam now measures the full replay corpus
- the ambient live-routing denominator is real, not curated
- the current routed families still reach only a tiny fraction of natural
  replay-corpus traffic

### Promoted-Family Probe

This is the promoted-family Calc-backed probe over the same replay corpus:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1379`
- `interpret_tail_authoritative_fallback_total=109`
- promoted-family authoritative rate: `92.67%`

Current promoted-family fallback reasons:

- `unsupported_formula_shape=26`
- `shadow_mismatch=64`
- `unsupported_host_surface=19`

Interpretation:

- once the probe hits promoted-family cells, authority conversion is now
  strong
- the main remaining conversion work is no longer generic breadth
- the current hotspots are residual mismatch and host-surface cleanup on the
  already-promoted families

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

The next high-value pass should focus on ambient live-routing conversion, not
another broad promoted-family wave.

Priority order:

1. increase the full-corpus `interpret_tail_live_seen_total`
2. convert current `unsupported_formula_shape` fallout on natural replay
   traffic
3. reduce retained `shadow_mismatch` and `unsupported_host_surface` on the
   promoted lookup and index family
4. only then widen to the next capability class

## Historical Archive

The retired pass-by-pass ledger lives in:

- [../archive/interpret_tail/](../archive/interpret_tail/)

