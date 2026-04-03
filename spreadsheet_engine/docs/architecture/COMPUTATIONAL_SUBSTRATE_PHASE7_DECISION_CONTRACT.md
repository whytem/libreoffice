# Computational Substrate Phase 7 Decision Contract

Status: active contract note for Phase 7

## Purpose

This note freezes the exact decision surface for Phase 7 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

It exists so the final host-boundary re-cut and rollout decision cannot drift
wider than the surfaces actually proven through Phase 6.

## Entry Boundary

Phase 7 begins only from the admitted Phase 6 authority surface:

- the admitted Phase 5 scalar lifecycle-authoritative subset remains the
  foundation
- structural authority is admitted only for:
  - single-sheet `InsertRows`
  - single-sheet `DeleteColumns`
  - the ordinary-scalar-formula slice with no shared groups or named ranges
- queue verification is exact
- computational verification is exact
- graph verification is exact
- reference-update divergence is treated as repair-detected and unacceptable
- rejection, rollback, and repair-detected outcomes remain first-class

This contract does not permit Phase 7 to treat broader structural classes as
already proven.

## Allowed Decision Outcomes

Phase 7 may close with only one of these outcomes:

- broad proceed
  - recommend rollout on a clearly defined long-term candidate boundary
- narrow proceed
  - recommend rollout or continued adoption only on a smaller candidate
    boundary than the full computational-substrate target
- stop
  - conclude that the computational-substrate program should remain an
    exploratory architecture effort rather than moving into broader rollout

Any final decision that does not fit one of those outcomes is out of contract.

## What Counts As A Valid Boundary Re-Cut

A valid Phase 7 boundary re-cut must:

- name the intended engine-owned computational surfaces explicitly
- name the intentionally retained Calc-owned host surfaces explicitly
- classify what remains deferred
- avoid hidden dual authority between Calc and `spreadsheet_engine/`
- define how rollout would be enabled, narrowed, or disabled

Phase 7 is not allowed to rely on vague "future extraction" language in place
of an explicit ownership map.

## Mandatory Evidence Categories

No rollout recommendation is credible unless all of the following evidence
categories are addressed:

- correctness on the admitted authority subset
- representative end-to-end load, edit, and recalc scenarios
- memory observations on selected workloads
- recalc-performance observations on selected workloads
- operational clarity:
  - debuggability
  - rollback behavior
  - understandable host and engine ownership
- standing zero-fallback replay health

## Hard Contract Limits

Phase 7 must not:

- widen structural authority beyond the Phase 6 admitted subset without new
  evidence
- treat retained host-owned document or environment services as extraction
  failures
- recommend rollout if the resulting boundary is harder to explain than the
  current one
- hide stop conditions behind optimistic language about future phases

## Required Closeout Shape

Phase 7 must finish with:

- one explicit ownership map
- one explicit retained host-surface list
- one explicit rollout matrix
- one explicit rollback or deactivation strategy
- one final decision record choosing broad proceed, narrow proceed, or stop
