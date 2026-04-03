# Computational Substrate Phase 4 Authority Schema

Status: active Phase 4 schema artifact

## Purpose

This note records the engine-owned authority state model introduced for
Phase 4 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

The goal of the schema is to make the first authority pilot explicit before
the project wires any live Calc apply path to it.

## Ownership Rules

The Phase 4 authority schema is defined in
[AuthorityPilot.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilot.hxx).

It uses the following ownership rules:

- computational state is carried by the engine-owned computational shadow
- graph state is carried by the engine-owned dependency-graph shadow
- execution-facing formula state is carried by the engine-owned IR shadow
- dependency and recalc answers are carried by engine-owned
  `DependencySnapshot`, `InvalidationPlan`, and `RecalcPlan` values
- mutation classification, verification modes, verdicts, and rollback state
  are value-semantic engine-owned enums and records

The authority schema may consume Calc-hosted adapters later, but it must not
persist:

- `ScDocument*`
- `ScFormulaCell*`
- `ScTokenArray*`
- listener or broadcaster container pointers
- formula-tree node pointers

## Core Types

The current Phase 4 schema contains:

- `AuthorityMutationClass`
- `AuthorityVerificationMode`
- `AuthorityPilotVerdict`
- `AuthorityPilotContract`
- `AuthorityPilotVerification`
- `AuthorityPilotInput`
- `AuthorityPilotTransition`

It also defines the first contract helpers:

- `classifyAuthorityMutation(...)`
- `makeAuthorityVerification(...)`

## Schema Semantics

The current schema is intentionally narrow:

- mutation admission is explicit and separate from later apply behavior
- verification mode is explicit per graph, queue, and IR surface
- rollback is carried as part of the transition record rather than implied by
  ad hoc runtime branches
- admitted and validation-only mutation classes are separate, so later phases
  can distinguish pilot authority from continued differential evidence

This means Phase 4 code can later distinguish:

- out-of-contract mutation attempts
- dirty-baseline rejection
- applicable authority transitions
- successfully applied transitions
- normalized-equivalent success
- rolled-back transitions

without reintroducing Calc-side hidden state as the real authority model.
