# Computational Substrate Phase 7 Retained Host Surfaces

Status: active retained host-surface note for Phase 7

## Purpose

This note records the host-owned surfaces that should remain in Calc even if
the computational substrate program proceeds into a bounded rollout.

It exists so the Phase 7 decision is not framed as "engine owns everything it
can reach." The intended boundary is cleaner when some services remain
explicitly host-owned.

## Retained Host Surfaces

The candidate retained host-owned surfaces are:

- `ScDocument` as the document and application host
- user-visible mutation APIs and document-shell behavior
- UI, UNO, rendering, import/export, and persistence
- external-reference cache ownership and session lookup
- printer, path, number-format, and similar environment services
- null-date acquisition and holiday-input expansion for host-owned add-in
  callers
- host-heavy inspection and environment services, including retained
  `INFO(...)` behavior
- formula-cell object lifetime while the computational substrate rollout is
  still bounded
- listener and broadcaster container storage while the broader live substrate
  remains only partially migrated
- retained `ScTokenArray` and stack-cursor ownership for deferred mutation
  classes and unreduced Calc-local token plumbing

## Why These Stay In Calc

These surfaces stay in Calc for one of three reasons:

- they are application-host concerns rather than portable spreadsheet
  semantics
- they are document-service integrations whose ownership is clearer in Calc
- broader substrate migration has not yet proven a cleaner replacement

## What This Means For Rollout

Any Phase 7 rollout recommendation must preserve this rule:

- engine authority may widen only where it makes the computational boundary
  cleaner
- retained host surfaces should stay explicit rather than being pulled into
  the engine opportunistically

This retained-host list is therefore part of the desired end state, not merely
an inventory of unfinished work.
