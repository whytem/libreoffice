# Computational Substrate Formula-Cell Lifetime Schema

Status: frozen schema artifact

## Purpose

This note records the engine-owned storage shape for the first
formula-cell-lifetime pilot defined in
[COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md).

The goal of the schema is to make admitted formula-cell lifetime explicit
before the project wires any Calc realization path to it.

## Ownership Rules

The admitted lifetime schema is intentionally narrow.

It uses the following identity and ordering rules:

- formula-cell lifetime identity is the normalized `ShadowCellId`
- admitted formula-cell order is lexical by normalized address
- formula-cell replace identity is the pair of:
  - normalized address
  - formula source text
- workbook generation remains an engine-owned scalar attached to the resident
  lifetime store, not a Calc-only side channel

The schema may be bootstrapped from Calc-backed observation, but it must not
persist:

- `ScDocument*`
- `ScFormulaCell*`
- `ScTokenArray*`
- formula-tree node pointers
- listener or broadcaster pointers
- any other Calc object identity as durable lifetime identity

## Core Records

The admitted formula-cell lifetime schema is expected to contain the
following logical records:

- an admitted formula-cell lifetime record carrying:
  - normalized cell identity
  - formula source text
- a resident lifetime-store wrapper carrying:
  - workbook generation
  - admitted formula-cell lifetime records
  - stable ordering and lookup helpers

The schema is deliberately narrower than the full computational shadow:

- scalar payload ownership remains part of resident cell storage
- formula-tree and formula-track realized order remain part of resident
  wiring containers
- dirty, recalc, and dependency-side status remain part of the broader
  mutable computational and graph surfaces
- formula-cell object identity is semantic and value-based, not pointer-based

## Comparison Semantics

The admitted formula-cell lifetime comparison model is:

- exact admitted formula-cell population match is required
- exact admitted formula-cell source match is required
- exact workbook generation match is required

Normalized-equivalent success is intentionally narrow in this pilot:

- Calc may realize different `ScFormulaCell*` object identities
- Calc may allocate different transient host storage
- but the realized admitted formula-cell surface must still compare equal
  after normalization to the resident lifetime store

In other words, admitted formula-cell lifetime equality is semantic and
value-based, not pointer-based.

## Explicit Non-Admission

The schema does not admit or represent:

- shared-group members
- matrix formulas
- named-range-sensitive or scope-ambiguous lifetime behavior
- off-sheet structural spill classes
- sheet-level structural lifetime classes
- external-reference-sensitive formula-cell lifetime
- broad `ScDocument` object-lifetime ownership

Those remain outside the first formula-cell-lifetime pilot and must not be
smuggled in through a broader record definition.

## Working Consequence

This schema is narrow on purpose:

- the engine can become authoritative for admitted formula-cell lifetime
- Calc can realize that resident lifetime state into the retained live
  document model
- and later migration steps can reason separately about:
  - direct mutation entry
  - broader structural classes
  - broad document object-lifetime migration

without forcing the first lifetime pilot to solve every remaining live object
management concern at once.
