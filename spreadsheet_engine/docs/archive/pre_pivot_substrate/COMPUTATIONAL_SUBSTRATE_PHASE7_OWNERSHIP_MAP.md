# Computational Substrate Phase 7 Ownership Map

Status: completed Phase 7 ownership map

## Purpose

This note records the candidate long-term Calc and engine boundary using the
surfaces actually proven through Phase 6.

It classifies each major computational surface into one of four states:

- ready for engine authority
- ready only for bounded rollout
- intentionally host-owned
- deferred

## Ready For Engine Authority

These surfaces are already stable enough to treat as engine-owned in the
long-term boundary:

- shared compiler and compile-host interfaces
- standalone workbook loading and sparse workbook modeling
- standalone evaluation semantics and shared runtime helpers
- dependency snapshot construction
- invalidation planning
- recalc planning and queue construction
- execution-facing IR and token modeling as the engine-side semantic model
  for the admitted subset
- compat and direct-entry adapters that package host services around
  engine-owned spreadsheet semantics

These are not speculative future candidates. They are already part of the
effective engine-owned computation layer.

## Ready Only For Bounded Rollout

These surfaces are proven enough to support a bounded rollout candidate, but
not broad authority transfer:

- computational storage shadow and lifecycle state on the admitted scalar
  lifecycle subset
- formula-tree membership and formula-track behavior on the admitted scalar
  lifecycle subset
- dependency-graph update and recalc-queue authority on the admitted mutation
  subset
- structural and reference-update authority for:
  - single-sheet `InsertRows`
  - single-sheet `DeleteColumns`
  - the ordinary-scalar-formula slice with no shared groups or named ranges
- rollback, rejection, and repair-detected behavior on the admitted subset

The common rule for these surfaces is:

- they are credible for bounded authority
- they are not yet credible for blanket document-wide authority

## Intentionally Host-Owned

These surfaces should remain Calc-owned in the candidate long-term boundary:

- UI, UNO, rendering, shell integration, and persistence
- document hosting and user-visible mutation APIs
- printer, path, number-format, and environment services
- external-reference cache ownership and session lookup services
- null-date acquisition and holiday-input expansion for host-owned add-ins
- host-heavy inspection and environment services such as `INFO(...)`
- formula-cell object lifetime as long as broader substrate rollout remains
  bounded
- listener and broadcaster container storage as long as the bounded rollout
  does not replace full document storage ownership

Retaining these surfaces in Calc is a deliberate boundary choice, not leftover
cleanup debt.

## Deferred

These surfaces are not ready for broad engine authority and should remain
deferred after Phase 7 unless new evidence appears:

- shared-group creation, split, merge, and repair
- named-range-sensitive structural authority in the live path
- sheet insert, delete, rename, and move authority
- copy, move, clipboard, load-time, and undo-like structural behavior
- broad range, union, and intersection authority beyond the admitted subset
- `ScTokenArray` ownership transfer
- Calc stack-container mutation and token-cursor ownership migration
- full computational table and column storage replacement across all workbook
  classes

These defer items are the main reason the current recommended outcome is more
likely to be a narrow rollout than a broad one.

## Boundary Summary

The cleanest candidate boundary after Phase 6 is:

- engine owns spreadsheet semantics, dependency and recalc planning, the
  engine-side execution model, and the bounded admitted authority slices
- Calc hosts the document, environment services, object lifetime, and the
  broader structural and token-container surfaces that remain deferred

That boundary is coherent because it does not pretend the bounded admitted
substrate slice has already grown into universal document authority.
