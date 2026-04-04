# Computational Substrate Cell Storage Schema

Status: frozen schema artifact

## Purpose

This note records the engine-owned storage shape for the first
cell-storage-residency pilot defined in
[COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md).

The goal of the schema is to make admitted cell residency explicit before the
project wires any Calc mirroring path to it.

## Ownership Rules

The admitted storage schema is intentionally narrow.

It uses the following ownership rules:

- admitted cell identity is the normalized `api::CellAddress`
- admitted storage order is lexical by `(sheet, column, row)`
- scalar payload identity is the stored `api::CellValue`
- formula payload identity is the pair of formula source text and cached value
- workbook generation remains an engine-owned scalar attached to the resident
  store, not a Calc-only side channel

The schema may be bootstrapped from Calc-backed observation, but it must not
persist:

- `ScDocument*`
- `ScFormulaCell*`
- `ScTokenArray*`
- `ScBaseCell*`
- broadcaster or listener container pointers
- any other Calc object identity as durable storage identity

## Core Records

The admitted cell-storage schema is expected to contain the following logical
records:

- an admitted cell-store record keyed by normalized address
- a scalar cell record carrying address and `api::CellValue`
- a formula cell record carrying:
  - address
  - formula source text
  - cached value
  - ordinary-formula classification only
  - dirty and recalc metadata when present on the admitted slice
- a resident-store wrapper carrying:
  - workbook generation
  - admitted cells
  - stable ordering helpers

The schema is deliberately narrower than the full computational shadow:

- named-range descriptors are not part of resident cell identity here
- graph and wiring targets remain separate engine-owned surfaces
- formula-tree and formula-track membership remain separate engine-owned
  surfaces
- live listener and broadcaster container residency remains separate

## Comparison Semantics

The admitted cell-storage comparison model is:

- exact address match is required
- exact scalar value match is required
- exact formula source match is required
- exact cached-value match is required
- exact cell-kind match is required

Normalized-equivalent storage outcomes are intentionally minimal in this
pilot. A result should count as full success only when the engine-resident
store and the Calc-mirrored admitted cells describe the same after-state
without depending on Calc-only object identity.

## Explicit Non-Admission

The schema does not admit or represent:

- shared-group members
- matrix formulas
- sheet-local or ambiguous named-range cell classes
- off-sheet structural expansion
- external-reference-sensitive cell storage
- undo or clipboard-specific storage identity
- live broadcaster/listener container residency

Those remain outside the first cell-residency pilot and must not be smuggled
in through a broader record definition.

## Working Consequence

This schema is narrow on purpose:

- the engine can become authoritative for admitted cell residency
- Calc can mirror that resident state into the retained live document model
- and later migration steps can reason separately about:
  - graph and wiring residency
  - formula-cell object lifetime
  - broader structural and named-range classes

without forcing the first cell-storage pilot to solve every remaining
computational-document problem at once.
