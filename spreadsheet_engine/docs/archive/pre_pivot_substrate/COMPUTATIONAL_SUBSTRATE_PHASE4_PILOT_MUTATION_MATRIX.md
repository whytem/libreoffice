# Computational Substrate Phase 4 Pilot Mutation Matrix

Status: active Phase 4 mutation matrix

## Purpose

This note records the exact mutation classification for the first
engine-authoritative pilot in Phase 4.

It exists so runtime guards, tests, and later decision records all refer to
the same mutation boundary.

## Admitted Authoritative Mutations

The following mutation classes are admitted for authoritative handling in
Phase 4:

### `SetValue`

- status: admitted
- authority expectation: exact or explicitly normalized-equivalent graph and
  queue result
- required baseline: clean formula-tree / queue state
- verification lane: authoritative dependency-shadow pilot tests

### scalar `SetString`

- status: admitted
- authority expectation: exact or explicitly normalized-equivalent graph and
  queue result
- required baseline: clean formula-tree / queue state and no formula parse
  widening beyond the admitted subset
- verification lane: authoritative dependency-shadow pilot tests

### formula edit via `SetString`

- status: admitted
- authority expectation: exact or explicitly normalized-equivalent graph and
  queue result on the admitted IR-backed subset
- required baseline: clean formula-tree / queue state
- verification lane: authoritative dependency-shadow pilot tests plus
  workbook-facade / compile-diff cases where needed

### direct `SetFormula`

- status: admitted
- authority expectation: exact or explicitly normalized-equivalent graph and
  queue result on the admitted IR-backed subset
- required baseline: clean formula-tree / queue state
- verification lane: authoritative dependency-shadow pilot tests plus
  workbook-facade / compile-diff cases where needed

### `ClearCell`

- status: admitted
- authority expectation: exact or explicitly normalized-equivalent graph and
  queue result
- required baseline: clean formula-tree / queue state
- verification lane: authoritative dependency-shadow pilot tests

## Validation-Only Mutations

The following mutations remain part of the comparison surface but are not
admitted as authoritative Phase 4 mutations:

### named-range rename

- status: validation-only
- reason: admitted in prior shadow phases, but not yet narrow enough for the
  first authority pilot

### representative row insert

- status: validation-only
- reason: representative structural widening remains evidence for later phases,
  not Phase 4 authority

### representative column delete

- status: validation-only
- reason: representative structural widening remains evidence for later phases,
  not Phase 4 authority

## Rejected Or Deferred Mutations

The following mutation classes must be rejected or skipped by Phase 4
authority:

- delayed-listener startup cases
- delayed-broadcaster deletion cases
- broad structural edits
- copy or move mutations
- clipboard mutations
- load-time or `CalcAfterLoad` repairs
- undo-driven lifecycle repair paths
- host-heavy external-reference or document-service mutations

## Expected Verdict Categories

Every authoritative Phase 4 mutation attempt must end in one of the following
explicit categories:

- `Applied`
- `NormalizedEquivalent`
- `RolledBack`
- `RejectedOutOfContract`
- `RejectedDirtyBaseline`

No mutation is allowed to fall back silently to Calc authority while still
being reported as a successful Phase 4 authority application.
