# Computational Substrate Global Named-Range Implementation

Status: completed implementation note for the global named-range admission
plan

## Purpose

This note records what the admission plan's live-candidate workstream actually
wired.

The goal of this phase was not automatic rollout widening. It was to let the
bounded global single-area named-range slice run through the existing
structural authority harness as a separately gated live candidate.

## What Landed

The implementation now extends the structural authority path in three specific
ways:

- the structural pilot can upgrade the bounded global single-area named-range
  slice from `ValidationOnly` to `Admitted` when an explicit candidate gate is
  enabled
- the ordinary structural rollout remains unchanged by default
- Calc and standalone tests now cover both the gated happy path and the
  gate-off rejection path

The main code surfaces are:

- [ComputationalSubstrateRollout.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateRollout.hxx)
- [ComputationalSubstrateStructural.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateStructural.hxx)
- [StructuralPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/StructuralPilotBuilder.hxx)
- [computational_substrate_tests.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/tests/unit/computational_substrate_tests.cxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

## Candidate Gate Shape

The bounded global named-range slice is now controlled by an explicit sub-gate:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_GLOBAL_NAMED_RANGE`

This gate does not inherit from the umbrella rollout gate by itself. The live
candidate path requires both:

- structural rollout capture to be enabled
- the dedicated global named-range gate to be enabled

That keeps the current admitted rollout surface stable while still allowing
the candidate slice to be exercised in the exact-rollback harness.

## Working Rules

The live-candidate path still preserves the current rollout discipline:

- the ordinary structural rollout remains the default admitted surface
- non-global, local, multi-area, and ambiguous cases still reject
- exact queue, computational, and graph verification remain mandatory
- repair-detected rollback remains mandatory

The global named-range candidate therefore uses the same authority bridge as
the existing structural rollout, but only after passing the extra gate.

## Result Of This Workstream

Phase 4 of the plan now leaves the admission cycle in a useful state:

- the bounded global single-area named-range slice can run as a separately
  gated live candidate
- the gate-off path still proves that named-range-sensitive behavior is not
  silently admitted by the ordinary structural rollout
- the remaining admission question is now evidence-driven rather than blocked
  on missing runtime wiring
