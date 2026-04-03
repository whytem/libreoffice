# Computational Substrate Phase 6 Decision Record

Status: complete closeout decision for Phase 6

## Decision

Proceed to Phase 7, but only on the narrower structural-authoritative subset
actually validated in Phase 6.

Phase 6 proved that `spreadsheet_engine/` can be the real source of a first
structural and reference-update answer on top of the admitted scalar lifecycle
surface from Phase 5. It did not prove broad structural authority for Calc
documents.

The Phase 6 result is therefore a narrow proceed decision rather than a broad
authority admission.

## What Phase 6 Now Admits

The admitted structural-authoritative subset is:

- `InsertRows` on a single sheet
- `DeleteColumns` on a single sheet
- clean-baseline entry only
- workbook slices that contain only ordinary scalar formulas
- no shared-group membership before or after the mutation
- no named ranges in the admitted structural slice
- engine-authored reference-update expectations for the admitted shifted
  scalar formulas
- exact queue verification after application
- exact computational verification after application
- exact graph verification after application
- explicit rejection, rollback, and repair-detected verdicts

On that admitted subset, the engine now owns:

- structural mutation classification
- post-mutation computational population expectations
- admitted reference-update expectations for shifted scalar formulas
- engine-authored post-mutation graph and recalc answers
- explicit host synchronization actions for the admitted mutation subset
- explicit divergence handling when Calc widens, repairs, or otherwise
  disagrees with the admitted answer

Calc still owns:

- document storage and structural mutation APIs
- formula-cell object lifetime
- listener and broadcaster container storage
- the live host verification and rollback surface

That is acceptable for Phase 6 because the phase goal was not full structural
ownership transfer. It was to prove that the engine can originate the admitted
structural answer and detect when Calc diverges from it.

## What Remains Deferred

The following remain deferred after Phase 6:

- `DeleteRows`
- `InsertColumns`
- named-range-sensitive structural authority in the live path
- shared-group creation, split, merge, or repair
- sheet insert, delete, rename, or move authority
- copy, move, clipboard, load-time, and undo-like structural behavior
- broader range/union/intersection widening beyond what is directly needed by
  the admitted row-insert and column-delete subset
- any structural path whose correctness still depends on retained Calc-owned
  repair beyond the admitted rollback and verification bridge

These are not partially admitted by implication. They remain explicit defer
surfaces for later phases.

## Verification And IR Gate Decision

Phase 6 keeps a stricter contract than the Phase 5 lifecycle pilot:

- queue comparison is exact
- computational comparison is exact
- graph comparison is exact
- structural reference-update divergence is treated as repair-detected and
  unacceptable on the admitted subset

Execution-IR comparison should still be treated as observational program data
in Phase 7 rather than as a new generalized normalized-equivalence regime.

Phase 6 does use the predicted IR/reference-update answer to detect silent
structural repair on the admitted subset, but it does not broaden the
authority contract into a standalone IR-first runtime gate beyond that narrow
structural verification role.

## Evidence

The checked-in Phase 6 artifacts are:

- [COMPUTATIONAL_SUBSTRATE_PHASE6_STRUCTURAL_CONTRACT.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE6_STRUCTURAL_CONTRACT.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE6_STRUCTURAL_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE6_STRUCTURAL_MATRIX.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE6_STRUCTURAL_SCHEMA.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE6_STRUCTURAL_SCHEMA.md)
- [COMPUTATIONAL_SUBSTRATE_PHASE6_DIFFERENTIAL_SURFACE.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE6_DIFFERENTIAL_SURFACE.md)
- [StructuralPilot.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/StructuralPilot.hxx)
- [StructuralPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/StructuralPilotBuilder.hxx)
- [ComputationalSubstrateStructural.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateStructural.hxx)

## Validation Summary

Phase 6 closeout validation is green on:

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

## Proceed Boundary For Phase 7

Phase 7 may proceed, but only from this narrower boundary:

- the admitted Phase 5 scalar lifecycle subset remains the foundation
- structural authority is admitted only for:
  - single-sheet row insert
  - single-sheet column delete
  - the ordinary-scalar-formula slice with no shared groups or named ranges
- exact queue, computational, and graph verification remain mandatory
- rejection, rollback, and repair-detected outcomes remain first-class
- Phase 7 should widen structural authority incrementally rather than treating
  Phase 6 as evidence for broad structural ownership

If Phase 7 cannot preserve those guardrails while attempting a broader
host-boundary re-cut, the program should narrow again rather than treating
Phase 6 as blanket evidence for full structural authority.
