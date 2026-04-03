# Computational Substrate Narrow Rollout Decision Record

Status: complete closeout decision for the narrow rollout plan

## Decision

Widen the opt-in narrow rollout by one bounded step.

The rollout does not stay on the original structural subset from the Phase 7
decision. It now admits the two newly proven structural classes:

- `DeleteRows`
- `InsertColumns`

The rollout also does not broaden beyond that. It remains:

- opt-in
- exact-verification-based
- rollback-capable
- limited to the ordinary-scalar-formula slice

## Final Admitted Rollout Surface

The completed narrow rollout surface is:

- admitted scalar lifecycle authority
- single-sheet `InsertRows`
- single-sheet `DeleteRows`
- single-sheet `InsertColumns`
- single-sheet `DeleteColumns`
- ordinary scalar formulas only
- clean baseline only
- no shared groups
- no named-range-sensitive structural behavior
- exact queue verification
- exact computational verification
- exact graph verification
- repair-detected rollback on structural divergence

Calc still owns:

- document storage and mutation APIs
- formula-cell object lifetime
- listener and broadcaster container storage
- final live host verification and rollback mechanics

That is acceptable here because this rollout is still a bounded compat-driven
authority slice, not a blanket storage migration.

## Why The Rollout Widens

The widening decision is justified because both candidate mutation classes now
have the same proof shape as the already-admitted structural slice:

- exact standalone structural transition prediction
- exact standalone IR/reference-update prediction
- exact Calc differential happy-path application
- deterministic dirty-baseline rejection
- deterministic out-of-slice rejection
- deterministic repair-detected rollback

No current evidence shows that either candidate depends on hidden Calc repair
on the admitted scalar slice.

## What Remains Deferred

The rollout still does not admit:

- shared-group-sensitive structural behavior
- named-range-sensitive structural behavior
- sheet insert, delete, rename, or move
- copy, move, clipboard, load-time, or undo-like structural flows
- broader storage migration
- token-container ownership transfer

Any widening beyond the current surface requires a new explicit plan and
proof cycle.

## Evidence

The closeout evidence for this decision is:

- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_IMPLEMENTATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_IMPLEMENTATION.md)
- [COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_EVIDENCE.md)
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_EVIDENCE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_EVIDENCE.md)

## Validation Summary

Closeout validation is green on:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_ir_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Standing replay baseline remains:

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

## Result

The narrow rollout plan is complete.

Its final outcome is:

- not stop
- not broad rollout
- widen the bounded opt-in rollout to include `DeleteRows` and
  `InsertColumns` alongside the previously admitted lifecycle, `InsertRows`,
  and `DeleteColumns` surfaces

