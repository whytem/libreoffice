# Computational Substrate Mutation Entry Schema

Status: frozen admitted mutation-entry schema

## Purpose

This note defines the engine-owned mutation-entry shape for
[COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md).

The schema is intentionally small. It only needs to represent the admitted
mutation-entry slice that is already bounded by the mutation-entry contract.

## Engine-Owned Request Shape

The engine-owned admitted request shape should model:

- request kind
- target address or target sheet/span
- scalar payload when the request is scalar-valued
- formula source when the request is formula-valued

That means the engine should receive mutation intent in a normalized form
that does not depend on Calc-local stack state or token-container state.

## Stable Identity Rules

Stable mutation-entry identity in this cycle is:

- cell address for single-cell lifecycle requests
- sheet id plus row start/count for row structural requests
- sheet id plus column start/count for column structural requests

The schema does not admit:

- token-array identity
- transient broadcaster/listener object identity
- Calc-local pointer identity

## Request Routing Rules

The engine-owned request shape must route each admitted request into exactly
one of these admitted paths:

- authority path for scalar-value mutations
- lifecycle path for formula insert/replace/remove mutations
- structural path for the already-admitted narrow row/column edits

This routing is part of the engine-owned entry boundary. It must not be left
implicit in scattered Calc call sites.

## Payload Rules

The schema admits the following payload categories:

- scalar value payloads for `SetScalarValue`
- formula source payloads for `SetFormula`
- empty payload for `ClearCell`
- count payloads for admitted row/column structural edits

The schema intentionally does not admit:

- clipboard payloads
- token-container payloads
- shared-group payloads
- named-range mutation payloads

## Exact Comparison Rules

Exact mutation-entry success is measured against:

- engine-owned resident cell storage after-state
- engine-owned resident formula-cell lifetime after-state
- engine-owned resident wiring after-state
- engine-owned graph and queue after-state
- Calc realized live after-state after engine-issued apply/realization

Normalized-equivalent verdicts may still exist in lower-level machinery, but
this cycle should still treat exact queue, computational, and graph agreement
as the admitted target.

## Forbidden Shortcuts

The following shortcuts are out of contract for this schema:

- using a Calc-local mutation site as the hidden source of truth for request
  routing
- inferring admitted mutation identity from token-container ownership
- inferring resident after-state solely from live Calc object residue
- treating host-side repair as equivalent to engine-owned entry authority

## Working Interpretation

The purpose of this schema is not to invent a broad new mutation language.
It is to give the engine a stable, value-semantic admitted request surface
for the exact narrow slice the project has already proven elsewhere.
