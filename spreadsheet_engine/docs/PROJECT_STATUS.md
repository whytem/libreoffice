# Spreadsheet Engine: Project Status

This file is the concise current-state snapshot for `spreadsheet_engine/`.

For the full computational-substrate boundary, admitted slice, roadmap, and
blockers, start with
[architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md).

## Project Objective

The long-term objective is to make `spreadsheet_engine/` the home for Calc's
spreadsheet-specific computation engine while keeping Calc as the document
and application host.

In practical terms, the engine is intended to own:

- formula compilation and compiler-host interfaces
- token modeling and execution-facing compiler output
- spreadsheet evaluation semantics
- dependency analysis, invalidation planning, and recalc planning
- standalone workbook loading and evaluation
- bounded admitted-slice authority and ownership where exact carry-through is
  proven

Calc is intended to remain responsible for:

- UI, import/export, rendering, UNO, and shell integration
- persistence and document-service integrations
- retained document-shell behavior outside the admitted slice

## Current Status At A Glance

The original extraction objective is effectively achieved.

Today:

- the engine builds standalone with CMake and also inside LibreOffice
- the engine owns the shared compiler and token-model surface
- the engine owns the standalone workbook model, FODS loader, parser, and
  evaluator
- the engine owns dependency snapshots, invalidation planning, and recalc
  planning
- Calc already consumes substantial engine-owned runtime and compat logic in
  production execution paths
- the computational-substrate program proved a bounded, exact, opt-in
  authority slice
- the program also proved a bounded ownership-complete admitted slice on top
  of that authority result

What has not been proven is equally important:

- broad default-on authority is not justified
- broad document-core replacement is not justified
- broad storage, listener, token-container, or workbook-wide ownership
  transfer is not justified

## Verified Baseline

The standing promoted replay baseline is:

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

Enabled promoted replay families include:

- `addin`
- `array`
- `database`
- `date_time`
- `financial`
- `information`
- `logical`
- `mathematical`
- `spreadsheet`
- `statistical`
- `text`

This baseline remains the hard guardrail for future widening work.

## Current Computational Substrate Position

### Engine-Owned Today

The engine broadly owns:

- compiler and token infrastructure
- standalone workbook loading and evaluation
- dependency snapshots, invalidation planning, and recalc planning
- a substantial shared runtime used by Calc

Within the admitted slice, the engine also owns bounded end-to-end decision
records for:

- sidecar computational state
- admitted resident cell and wiring state
- admitted formula-cell lifetime decisions
- object realization, rollback, raw mutation, and live-apply records
- primitive realization, rollback, and execution records

### Calc-Retained Today

Calc still intentionally owns:

- broad `ScDocument` storage and mutation outside the admitted slice
- broad object lifetime outside the admitted slice
- broad listener and broadcaster residency outside the admitted slice
- token-container construction and Calc-local token plumbing
- UI, UNO, rendering, persistence, and document-service integration
- the retained host shell that executes engine-authored records on the
  admitted live slice

## Current Opt-In Narrow Rollout Surface

The admitted rollout surface currently includes:

- admitted scalar lifecycle authority
- admitted scalar mutation entry
- single-sheet `InsertRows`, `DeleteRows`, `InsertColumns`, and
  `DeleteColumns`
- exact same-sheet shareable shared-group structural `Preserve`, `Split`, and
  `Rebuild`
- exact same-sheet shareable shared-group non-structural member-exit
  `SetScalarValue`, `SetFormula`, and `ClearCell`
- exact same-sheet shareable same-text preserve, bounded regroup, bounded
  merge, bounded replacement-merge, and bounded one-sided insertion
- exact same-sheet shareable named-range-combined `SameTextPreserve`
  `SetFormula`
- exact same-sheet shareable named-range-combined `MemberExit`
  `SetScalarValue`, `SetFormula`, and `ClearCell` on the bounded
  `GlobalSingleAreaSameSheet` surface

This remains an opt-in, exact-verification boundary with rollback on
divergence, not a broad live-authority flip.

## Explicitly Deferred Frontier

The following remain deferred:

- named-range-sensitive structural rollout
- broader named-range-combined regroup, merge, and collapse
- broader non-edge regroup and merge shared-group behavior
- repair-sensitive host normalization
- off-sheet shared-group consumers and dependency closure
- broad storage migration beyond the admitted slice
- broad token-container and listener ownership transfer
- workbook-wide or sheet-wide authority transfer

## Next Roadmap Category

The highest-value next category remains widening the admitted slice on the
same bounded surface before opening new host surfaces.

The recommended order is:

1. broader same-sheet named-range-combined regroup, merge, and collapse only
   when the engine can author the exact live after-state
2. broader same-sheet non-edge regroup and merge families
3. repair-sensitive normalization
4. bounded off-sheet shared-group consumers

The staged program plan for attacking those blockers together is
[architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md).

## Working Rules Going Forward

- treat the replay baseline as a non-negotiable guardrail
- widen only when the engine authors the exact live after-state
- prefer narrow, exact, well-proved slices over broad synthetic modeling
- keep rollback, verification, and retain/reject boundaries explicit
- treat individual historical `COMPUTATIONAL_SUB*.md` files as supporting
  detail, not the primary status surface

## Reference Material

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md):
  canonical computational-substrate reference
- [architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md):
  ambitious staged blocker-clearance roadmap
- [architecture/README.md](architecture/README.md): architecture-doc entry
  point
- [archive/](archive/): archived milestone plans and closeout material
- [extraction-history/](extraction-history/): older extraction and migration
  notes
