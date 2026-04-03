# Computational Substrate Phase 7 Decision Record

Status: complete closeout decision for Phase 7

## Decision

Proceed only on a narrower candidate boundary.

Phase 7 does not support a broad rollout of the full computational-substrate
target. It also does not justify stopping the program as a failed exploratory
effort.

The supported outcome is a narrow proceed:

- keep the successful first-stage extraction and shared-engine boundary as the
  stable base
- treat the admitted computational-substrate authority slices as a bounded
  rollout candidate
- do not recommend broader document-wide structural or storage authority yet

## Candidate Boundary Chosen

The candidate boundary justified by the evidence is:

- engine-owned:
  - shared compiler and compile-host interfaces
  - standalone workbook loading and evaluation semantics
  - dependency snapshots, invalidation planning, and recalc planning
  - engine-side execution IR and shared execution semantics
  - bounded authority for:
    - admitted scalar lifecycle mutations
    - admitted single-sheet `InsertRows`
    - admitted single-sheet `DeleteColumns`
    - the ordinary-scalar-formula slice with no shared groups or named ranges
- Calc-owned:
  - document hosting and user-visible mutation APIs
  - formula-cell object lifetime
  - listener and broadcaster container storage
  - external-reference cache ownership and session lookup
  - environment and host-service integrations
  - deferred structural, shared-group, and token-container surfaces

This is the narrowest boundary that still turns the computational-substrate
program into a meaningful product-direction result rather than only a
technical experiment.

## Why The Decision Is Narrow Proceed

The evidence does support:

- a real engine-authored authority slice
- deterministic rejection, rollback, and repair-detected behavior
- stable replay and standalone baselines
- a rollout candidate that remains understandable

The evidence does not support:

- broad structural authority across all workbook classes
- shared-group or named-range-sensitive rollout
- document-wide storage replacement
- `ScTokenArray` ownership migration
- a simpler broad boundary than the current narrow candidate

That is why broad proceed would overstate the proof surface, while stop would
understate the real value of the admitted authority slice.

## What Remains Intentionally Host-Owned

The retained host-owned boundary remains:

- `ScDocument` as document and application host
- user-visible mutation APIs
- UI, UNO, rendering, import/export, and persistence
- external-reference cache ownership and session lookup
- environment and host-service integrations
- formula-cell object lifetime while rollout remains bounded
- listener and broadcaster container storage while broader substrate rollout
  is deferred
- retained `ScTokenArray` and stack-cursor ownership for deferred mutation
  classes

These are part of the recommended boundary, not merely unresolved debt.

## What Remains Deferred

The following remain deferred after Phase 7:

- shared-group lifecycle and structural repair
- named-range-sensitive structural authority
- sheet insert, delete, rename, and move authority
- copy, move, clipboard, load-time, and undo-like structural behavior
- broad range, union, and intersection authority
- document-wide computational storage replacement
- `ScTokenArray` ownership transfer

No future work should treat these as implied rollout commitments without new
evidence.

## Rollout Recommendation

If rollout proceeds, it should proceed only as:

- opt-in
- developer or experimental first
- exact verification required
- rollback or deactivation required on divergence
- limited to the admitted workbook and mutation classes captured in
  [COMPUTATIONAL_SUBSTRATE_PHASE7_ROLLOUT_MATRIX.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE7_ROLLOUT_MATRIX.md)

That rollout shape is narrow enough to stay honest and broad enough to justify
continuing beyond pure architecture exploration.

## Validation Summary

Phase 7 closeout validation is green on:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_ir_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

Standing replay baseline remains:

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

## Program Outcome

The computational-substrate program is complete as a staged architecture
decision process.

Its outcome is:

- not broad rollout
- not stop
- narrow proceed on the bounded admitted authority slice

If future work continues, it should do so as bounded rollout engineering on
that candidate boundary rather than as another unconstrained architecture
expansion.
