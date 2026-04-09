# Computational Substrate Phase 4 Decision Record

Status: complete closeout decision for Phase 4

## Decision

Proceed to Phase 5, but only on the narrower graph-and-queue-authoritative
subset actually validated in Phase 4.

Phase 4 proved that `spreadsheet_engine/` can be the real source of
dependency-graph updates and recalc-queue construction for the admitted pilot
mutation subset while Calc still hosts document mutation, queue application,
verification capture, and rollback.

This is a genuine authority shift, but it is still a narrow one. Phase 4 did
not prove broad formula lifecycle authority, broad structural authority, or
execution-IR authority.

## What Phase 4 Now Admits

The admitted Phase 4 authoritative subset is:

- engine-authored dependency-graph updates for:
  - `SetValue`
  - scalar `SetString`
  - formula text edit via `SetString`
  - direct `SetFormula`
  - `ClearCell`
- engine-authored recalc-queue derivation for the same admitted mutation set
- Calc-side apply/verify/rollback bridging for that admitted set
- deterministic rejection of:
  - dirty baselines
  - validation-only mutation classes
  - rejected mutation classes
- deterministic rollback when live post-apply verification diverges on the
  queue-and-graph authority claim

The accepted verification contract is now:

- exact queue correspondence after application
- exact graph-facing verification after application
- explicit result categories for:
  - `Applied`
  - `AppliedNormalizedEquivalent`
  - `RolledBackVerificationFailure`
  - `RejectedDirtyBaseline`
  - `RejectedOutOfContract`

## What Remains Deferred

Phase 4 does not yet admit:

- engine-authoritative formula lifecycle
- listener or broadcaster storage ownership transfer
- named-range authority in the live authoritative path
- structural-edit authority in the live authoritative path
- hard rollback gating on execution-IR mismatch
- copy, move, clipboard, load-time, undo-like, or repair-heavy authority

Execution-IR comparison is still captured during the pilot, but in Phase 4 it
remains observation data rather than a hard rollback gate. That is deliberate
and should remain explicit in Phase 5 planning.

## Evidence

The checked-in Phase 4 artifacts are:

- [COMPUTATIONAL_SUBSTRATE_PHASE4_AUTHORITY_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE4_AUTHORITY_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE4_PILOT_MUTATION_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE4_PILOT_MUTATION_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE4_AUTHORITY_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE4_AUTHORITY_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE4_DIFFERENTIAL_SURFACE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE4_DIFFERENTIAL_SURFACE.md)
- [AuthorityPilot.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilot.hxx)
- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
- [ComputationalSubstrateAuthority.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx)

## Validation Summary

Phase 4 closeout validation is green on:

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
