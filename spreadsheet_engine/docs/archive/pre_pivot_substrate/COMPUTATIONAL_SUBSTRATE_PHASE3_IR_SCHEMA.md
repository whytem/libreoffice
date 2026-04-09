# Computational Substrate Phase 3 IR Schema

Status: active Phase 3 schema note

## Purpose

This note names the engine-owned schema introduced for Phase 3 of the
computational substrate program.

The schema is defined in
[ExecutionIr.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/ExecutionIr.hxx).

## Ownership Rules

The Phase 3 execution IR obeys these rules:

- durable IR identity is engine-owned and value-semantic
- Calc token-container identity is not durable IR identity
- Calc token-array addresses, pool positions, and container layout are not
  stored as authoritative IR state
- shared engine carriers such as reference data and opcode values may be
  reused where they are already stable engine-owned types
- build failures are explicit records on the workbook-level IR shadow rather
  than hidden fallback behavior

## Workbook-Level Shape

The workbook-level IR shadow is `ExecutionIrWorkbookShadow`.

It owns:

- snapshot metadata
- grammar
- lowered formula records for the admitted subset
- admitted formula-group descriptors carried forward from the computational
  shadow
- explicit lowering failures

## Formula-Level Shape

Each lowered formula is represented by `ExecutionIrFormulaRecord`.

It owns:

- formula-cell identity
- optional formula-group identity
- stored source text and namespace
- lowered instruction stream
- code-error and execution metadata
- formula-tree and formula-track membership flags

## Instruction-Level Shape

Each instruction is represented by `ExecutionIrInstruction`.

The instruction schema is intentionally separate from `token::Token`:

- it uses `ExecutionIrInstructionKind` instead of `token::Kind`
- it groups lowered markers into explicit IR instruction kinds
- it uses an IR-owned payload variant that reuses only stable engine carriers

## Admitted Payload Families

The initial Phase 3 schema admits:

- scalar literals
- string and string-name literals
- single and range references
- external single and range references
- column/row-name references
- range names, database ranges, and table references
- matrix literals
- jump tables
- byte payloads
- whitespace payloads
- opcode-bearing execution steps

## Explicit Defer Boundary

The schema note does not claim:

- whole-program execution authority
- full token-container migration out of Calc
- migration of listener, broadcaster, or BASM ownership
- authority for mutation surfaces outside the admitted Phase 3 subset

Those remain governed by the Phase 3 contract and the later decision record.
