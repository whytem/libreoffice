# Computational Substrate Phase 7 Correctness Evidence

Status: completed correctness evidence note for Phase 7

## Purpose

This note records the representative correctness scenarios used to support the
Phase 7 rollout decision.

It is not meant to replace the full validation contract. It exists to show
which scenario classes were persuasive enough to justify a narrow rollout
candidate instead of either a broad proceed or an immediate stop.

## Representative Admitted-Authority Scenarios

The representative admitted-authority scenarios exercised for Phase 7 are:

- scalar lifecycle insert on the admitted subset
  - lane: `testComputationalLifecycleInsertFormulaPilot`
  - validation surface: exact queue, computational, and graph verification
- structural row insert on the admitted subset
  - lane: `testComputationalStructuralInsertRowPilot`
  - validation surface: exact queue, computational, graph, and admitted
    reference-update behavior
- structural repair-detected rollback on the admitted subset
  - lane: `testComputationalStructuralRepairDetectedRollback`
  - validation surface: explicit repair detection plus deterministic rollback
- standalone computational substrate coverage
  - lane: `spreadsheetengine_computational_substrate_tests`
  - validation surface: engine-side transition and schema behavior without the
    Calc host bridge
- end-to-end promoted corpus replay
  - lane:
    `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
  - validation surface: load, parse, evaluate, and zero-fallback standing
    baseline across the promoted corpus

## What These Scenarios Show

Taken together, these scenarios support the following conclusions:

- the admitted scalar lifecycle surface is stable enough to remain part of a
  bounded rollout candidate
- the admitted structural surface is real authority, not just shadowing,
  because repair-detected divergence is classified and rolled back
- the engine-side substrate model remains coherent outside the Calc host
  bridge
- the broader promoted replay corpus remains intact while the computational
  substrate program continues

## Representative Host-Owned Scenarios

The following scenario classes remain intentionally outside the rollout
candidate and are therefore not treated as rollout blockers in this evidence
note:

- shared-group lifecycle and structural repair
- named-range-sensitive structural behavior
- external-reference-sensitive structural behavior
- load-time, clipboard, undo-like, and import-heavy structural flows
- environment and host-service paths such as retained `INFO(...)`,
  printer/path, null-date, and holiday packaging

These surfaces remain controlled by the retained host boundary rather than by
the bounded rollout candidate.

## Operational Clarity Assessment

The representative correctness scenarios support a narrow rollout better than
a broad one.

Why:

- the admitted subset is understandable and explicitly bounded
- rollback and repair-detected behavior are visible rather than hidden
- the boundary becomes harder to explain as soon as shared-group, named-range,
  or broader structural classes are assumed without new evidence

That assessment is a strong argument for narrow proceed and a strong argument
against broad proceed.
