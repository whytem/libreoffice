# Computational Substrate Wiring Container Schema

Status: frozen schema artifact

## Purpose

This note records the engine-owned storage shape for the first
wiring-container-residency pilot defined in
[COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md).

The goal of the schema is to make admitted live wiring residency explicit
before the project wires any Calc realization path to it.

## Ownership Rules

The admitted wiring schema is intentionally narrow.

It uses the following identity and ordering rules:

- listener-edge identity is the normalized ordered pair of:
  - `BroadcasterNodeId`
  - `ListenerAnchorId`
- broadcaster-node identity is the normalized `BroadcasterNodeId`
- formula-tree identity is the normalized `ShadowCellId`
- formula-track identity is the normalized `ShadowCellId`
- broadcaster-node order is lexical by normalized broadcaster identity
- listener-edge order is lexical by normalized edge identity
- formula-tree and formula-track order are the exact admitted realized orders
  produced by the engine-owned graph after-state
- workbook generation remains an engine-owned scalar attached to the resident
  wiring store, not a Calc-only side channel

The schema may be bootstrapped from Calc-backed observation, but it must not
persist:

- `ScDocument*`
- `ScFormulaCell*`
- `SvtBroadcaster*`
- `ScBroadcastArea*`
- broadcaster-store addresses
- listener-context container addresses
- formula-tree or formula-track node pointers
- any other Calc object identity as durable residency identity

## Core Records

The admitted wiring-container schema is expected to contain the following
logical records:

- an admitted broadcaster-node record carrying:
  - normalized broadcaster identity
  - admitted listener-count summary
- an admitted listener-edge record carrying:
  - normalized broadcaster identity
  - normalized listener-anchor identity
- a resident wiring-store wrapper carrying:
  - workbook generation
  - admitted broadcaster nodes
  - admitted listener edges
  - admitted formula-tree realized order
  - admitted formula-track realized order
  - stable ordering and lookup helpers

The schema is deliberately narrower than the full dependency graph:

- formula-group-sensitive wiring remains out of contract
- named-range-sensitive wiring remains out of contract
- off-sheet structural spill classes remain out of contract
- formula-cell object lifetime remains separate
- resident cell storage remains a separate engine-owned surface

## Comparison Semantics

The admitted wiring-container comparison model is:

- exact broadcaster-node population match is required
- exact listener-edge population match is required
- exact formula-tree realized order match is required
- exact formula-track realized order match is required
- exact workbook generation match is required

Normalized-equivalent success is intentionally narrow in this pilot:

- Calc realization may use different host object identities
- Calc realization may use different transient container allocation patterns
- but the realized admitted wiring surface must still compare equal after
  normalization to the resident wiring store

In other words, resident wiring equality is semantic and value-based, not
pointer-based.

## Explicit Non-Admission

The schema does not admit or represent:

- shared-group listener anchors or broadcasters
- matrix or shared formula wiring
- named-range-sensitive listener expansion
- sheet-local or scope-ambiguous name effects
- sheet insert, delete, rename, or move wiring classes
- external-reference-sensitive listener/container residency
- delayed host-only repair bookkeeping outside the admitted slice

Those remain outside the first wiring-container residency pilot and must not
be smuggled in through a broader record definition.

## Working Consequence

This schema is narrow on purpose:

- the engine can become authoritative for admitted live wiring-container
  residency
- Calc can realize that resident state into the retained live document model
- and later migration steps can reason separately about:
  - formula-cell object lifetime
  - broader named-range and shared-group surfaces
  - broader dependency-container replacement

without forcing the first wiring-container pilot to solve every remaining
live dependency residency concern at once.
