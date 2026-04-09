# Computational Substrate Phase 3 IR Contract

Status: active Phase 3 contract

## Purpose

This note freezes the exact semantic contract for the first engine-owned
execution IR introduced by Phase 3 of
[COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md).

It exists to prevent Phase 3 from drifting into a thin serialization wrapper
around `ScTokenArray` or, at the other extreme, an unbounded execution
replatforming effort.

## Admitted Surface

The first engine-owned execution IR is admitted only for the narrowed
computational-substrate subset already accepted by the Phase 2 decision:

- formula-bearing cells on the admitted graph-shadow subset
- admitted formula-group members and group anchors
- graph-facing execution metadata needed to relate lowered formulas back to:
  - formula tree membership
  - formula track membership
  - admitted formula-group identity
- rebuild-based mutation handling for:
  - `SetValue`
  - formula edit
  - formula insertion
  - `ClearCell`
  - named-range rename
- representative structural coverage for:
  - single row insert
  - single column delete

The Phase 3 IR is therefore an execution-facing representation for the
currently admitted formula population. It is not yet the authority for the
full Calc token universe or for the full document mutation surface.

## What Must Survive Lowering

The following concepts are required to survive lowering into the engine-owned
IR without relying on Calc token-container identity:

- scalar literals:
  - numbers
  - strings
  - errors
  - missing arguments
- opcode-bearing execution steps:
  - plain opcodes
  - lowered function-call markers
  - lowered unary-plus markers
  - lowered range-constructor markers
  - lowered reference-list markers
- reference carriers:
  - single references
  - double/range references
  - external single and double references
  - column/row-name references
- lookup and name carriers:
  - range names
  - database ranges
  - table references
  - external names
- execution-shape payloads:
  - matrix literals
  - jump tables
  - byte payloads
  - preserved whitespace
- formula metadata needed by later authority phases:
  - formula source text
  - code-error state
  - shareable/vector flags
  - hyperlink and threading metadata
  - formula-group membership
  - formula tree / formula track membership

## Intentional Normalization

Phase 3 is allowed to normalize current compiler output in the following ways:

- Calc token-container identity is discarded
- Calc pool indices, token-array addresses, and `ScTokenArray` storage order
  are not durable IR identity
- instruction identity is value-semantic and derived from engine-owned ids and
  payloads
- formula ordering may be compared by normalized formula-cell identity rather
  than build-order accident
- reference-update verdicts may use exact or normalized-equivalent comparison
  where the normalized rule is explicit and checked in

## Explicit Defers

The following remain outside the Phase 3 contract:

- full `ScTokenArray` migration out of Calc
- listener, broadcaster, or BASM authority transfer
- Calc listener-context migration
- copy/move/clipboard/load-time IR authority
- external-reference cache ownership
- host-heavy inspection or environment services
- whole-program execution authority for all formulas
- any requirement that the first IR slice model every Calc token behavior
  before the admitted subset is proven

## Authority-Candidate Meaning

For Phase 3, "execution authority candidate" means:

- the engine owns a durable execution-facing representation for the admitted
  subset
- current compiler output can be deterministically imported into that
  representation
- admitted reference-shape and representative structural-update behavior can
  be expressed at the IR boundary
- later phases can compare or pilot against that IR without treating
  `ScTokenArray` as the hidden source of truth

It does not mean:

- the IR is already the live execution authority
- the IR already replaces all Calc token-container behavior
- the project has already committed to migrating every remaining host-owned
  token or storage path

## Stop Condition

If Phase 3 cannot progress beyond a representation that merely stores imported
token payloads while depending on Calc token-container identity for meaning,
the correct outcome is to narrow or stop the program rather than claim an
engine-owned execution IR boundary that does not really exist.
