# Computational Substrate Phase 6 Structural Schema

Status: completed Phase 6 schema artifact

## Purpose

This note records the engine-owned state model introduced for Phase 6 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

The goal of the schema is to make the first structural-authority pilot
explicit before the project wires any live Calc structural bridge to it.

## Ownership Rules

The Phase 6 structural schema is defined in
[StructuralPilot.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/StructuralPilot.hxx).

It uses the following ownership rules:

- pre-mutation computational state is carried by the engine-owned
  computational shadow
- pre-mutation graph and execution-facing formula state are carried by the
  engine-owned graph and IR shadows
- observed post-mutation computational and IR state are recorded explicitly in
  the pilot input rather than hidden behind mutable Calc pointers
- dependency, invalidation, and recalc answers remain value-semantic engine
  records
- structural sync, reference-update summaries, verification modes, and
  verdicts are all value-semantic engine records

The schema may consume Calc-hosted adapters later, but it must not persist:

- `ScDocument*`
- `ScFormulaCell*`
- `ScTokenArray*`
- listener or broadcaster container pointers
- formula-tree node pointers

## Core Types

The current Phase 6 schema contains:

- `StructuralMutationClass`
- `StructuralVerificationMode`
- `StructuralPilotVerdict`
- `StructuralSyncActionKind`
- `StructuralPilotContract`
- `StructuralPilotVerification`
- `StructuralSyncAction`
- `StructuralReferenceUpdateRecord`
- `StructuralPilotInput`
- `StructuralPilotTransition`

It also defines the first contract helpers:

- `classifyStructuralMutation(...)`
- `makeStructuralVerification(...)`

## Schema Semantics

The current schema is intentionally narrow:

- only `InsertRows` and `DeleteColumns` are admitted structural mutations
- structural sync is modeled as explicit row or column operations, not as
  hidden document repair
- reference-update evidence is recorded per formula record through
  `StructuralReferenceUpdateRecord`
- observed after-state is explicit, which keeps the live structural bridge
  auditable and keeps later phases free to tighten or replace that evidence
  path
- rollback is carried as part of the transition record rather than implied by
  runtime side effects
