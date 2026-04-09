# Computational Substrate Phase 2 Special-Case Evidence

Status: active Phase 2 validation artifact

## Purpose

This note records the special-case and structural cases that Phase 2 admits and
validates before any Phase 3 proceed decision is made.

## Admitted And Validated In Phase 2

- delayed listener startup
- delayed broadcaster deletion
- representative single-row insert rebuild
- representative single-column delete rebuild

For these cases, the current requirement is:

- the engine-owned graph shadow remains comparable to live Calc state
- the comparison result is not `Mismatch`
- any normalization is explicit, not implicit

## Explicitly Deferred Beyond Phase 2

- broad structural authority beyond the representative row/column cases
- copy / move / clipboard rebuild authority
- load-time `CalcAfterLoad` graph authority
- BASM slot ownership
- Calc listener-context or broadcaster-container migration

These defer decisions are carried forward into the Phase 2 decision record so
the project either narrows honestly or proceeds on a clearly bounded subset.
