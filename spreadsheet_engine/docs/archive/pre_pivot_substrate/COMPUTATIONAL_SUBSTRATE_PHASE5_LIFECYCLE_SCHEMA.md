# Computational Substrate Phase 5 Lifecycle Schema

Status: active schema note for Phase 5

## Purpose

This note defines the engine-owned lifecycle state and sync model for the
first Phase 5 pilot.

## Core Records

Phase 5 introduces the following lifecycle-facing records:

- `LifecyclePilotContract`
- `LifecyclePilotVerification`
- `LifecyclePilotInput`
- `LifecycleSyncAction`
- `LifecyclePilotTransition`

These records are defined in:

- [LifecyclePilot.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/LifecyclePilot.hxx)

## Lifecycle Inputs

The lifecycle pilot starts from:

- admitted pre-mutation computational shadow
- admitted pre-mutation graph shadow
- admitted pre-mutation execution-IR shadow
- one normalized mutation event
- optional cached-value payload for formula-bearing post-state
- clean-baseline status

Calc pointers, broadcaster containers, and token-container identity are not
part of durable lifecycle pilot state.

## Sync Model

The engine emits explicit host synchronization work as `LifecycleSyncAction`
records.

Phase 5 currently models three sync actions:

- formula insertion
- formula replacement
- formula removal

Each sync action records:

- the target address
- whether a formula was present before and after
- formula source when the post-state is formula-bearing
- optional cached-value payload when the post-state is formula-bearing

## Verification Model

The lifecycle pilot verifies:

- computational post-state
- graph-facing post-state

Execution-IR comparison is still captured, but it remains observational in
Phase 5 unless later evidence changes the contract.

## Ownership Rule

The lifecycle model is engine-owned and value-semantic.

Calc remains the host that applies sync actions, captures live verification
state, and rolls back when the engine-authored answer does not hold.
