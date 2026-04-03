# Computational Substrate Phase 4 Authority Contract

Status: active Phase 4 contract

## Purpose

This note freezes the exact authority contract for Phase 4 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

It exists to keep the first engine-authoritative pilot narrow, explicit, and
auditable.

Phase 4 is allowed to make the engine authoritative only for the admitted
pilot surface recorded here. Everything else must remain shadow-only,
explicitly deferred, or explicitly rejected.

## Admitted Authority Surface

The engine may become authoritative only for the following mutation classes:

- `SetValue`
- scalar `SetString`
- formula text edit via `SetString`
- direct `SetFormula`
- `ClearCell`

Only formulas on the already admitted Phase 3 IR-backed subset are in scope.

That means the pilot may rely on:

- the completed computational shadow
- the completed dependency-graph shadow
- the completed execution-IR shadow
- deterministic lowering and comparison behavior already validated in Phase 3

The pilot must not silently widen beyond that surface.

## Preconditions

Phase 4 authority may run only when all of the following are true:

- the workbook is on the admitted IR-backed subset
- the formula-tree / queue baseline is clean before the mutation
- there is no pre-existing dirty or partially repaired computational debt
- the mutation does not require delayed listener startup handling
- the mutation does not require delayed broadcaster deletion handling
- the mutation does not require structural-edit authority
- the mutation does not depend on clipboard, copy/move, undo, or load-time
  repair behavior

If any precondition fails, Phase 4 must reject or skip authority explicitly.

## Accepted Authority Outputs

For admitted mutations, the engine authority answer consists of:

- the post-mutation graph-facing shadow state needed by the pilot
- the recalc-plan / queue output derived from that state
- the verification contract Calc must satisfy after application
- the explicit verdict category:
  - applicable
  - rejected as out-of-contract
  - rolled back after failed verification

Calc may host mutation application and queue execution in Phase 4, but it must
not decide the authoritative graph-and-queue answer first and only then compare
against the engine.

## Permitted Normalization

Phase 4 is allowed to accept normalized-equivalent outcomes only where the
normalization is explicit and already aligned to the completed shadow lanes.

The accepted normalization surface is:

- graph comparison rules already admitted in Phase 2
- IR comparison rules already admitted in Phase 3
- queue comparison rules that differ only by the already-documented
  normalized-equivalent ordering or grouping rules

Phase 4 is not allowed to widen normalization to hide:

- under-scheduling
- extra dirty entries without a checked-in contract reason
- missing verification steps
- hidden Calc-side repair after a failed authority answer

## Mandatory Rollback Triggers

Phase 4 must roll back or reject rather than apply authority when any of the
following occurs:

- the mutation is outside the admitted pilot matrix
- the baseline is not clean
- the post-apply verification state mismatches outside accepted normalization
- the resulting queue under-schedules or loses admitted graph edges
- the engine answer depends on a Calc-owned repair path that the pilot does
  not model directly

Rollback is part of the contract, not a best-effort debugging aid.

## Explicit Defers

The following remain outside the Phase 4 authority contract:

- named-range rename as an authoritative mutation
- row and column structural edits as authoritative mutations
- delayed listener startup and delayed broadcaster deletion authority
- copy, move, clipboard, undo, and load-time authority
- formula lifecycle authority
- listener or broadcaster storage authority
- direct execution from the engine-owned IR
- external-reference cache ownership
- host-heavy document-service integrations

These may remain in comparison or evidence lanes, but they are not admitted
authority mutations in Phase 4.

## Stop Condition

If the authoritative pilot needs Calc to stay the hidden graph-or-queue
authority even on the admitted mutation surface, the correct result is to
narrow or stop the program rather than claim Phase 4 authority.
