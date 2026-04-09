# Admitted Slice Ownership Closeout Implementation

Status: completed ownership-closeout implementation note

## Purpose

This note records the implementation shape for the engine-authored admitted
primitive host-call executor path used by
[ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md).

The goal of this implementation step is narrow:

- add one explicit engine-authored primitive host-call executor plan on the
  admitted slice
- classify that retained executor shell explicitly
- make the verified admitted result consume that new surface

This step does not widen the admitted slice and does not attempt broad
`ScDocument` independence.

## Landed Surfaces

The primitive host-call executor surface is now centered in:

- [ComputationalSubstratePrimitiveHostExecutor.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstratePrimitiveHostExecutor.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

The new compat surface adds:

- a value-semantic admitted primitive host-call executor plan
- stable host-call stage identity
- a bounded host-call observation classifier
- a runtime result surface for carrying that plan and observation through the
  admitted mutation-entry path

## Runtime Shape

The admitted runtime path now builds one additional explicit plan once the
already-settled upstream records exist:

- admitted raw document mutation record
- admitted primitive execution plan
- admitted primitive realization or primitive rollback record
- admitted final verification record

That executor plan then carries:

- the retained low-level raw document mutation host call
- the retained low-level realization or rollback host call
- the retained low-level final verification host call
- stable branch identity for apply versus rollback lanes

This keeps the remaining host shell explicit instead of leaving it implicit
inside Calc-local sequencing.

## Mutation Entry Integration

`MutationEntryResult` now carries:

- `moPrimitiveHostExecutorPlan`
- `moPrimitiveHostExecutorObservation`

Those surfaces are populated on:

- admitted apply lanes
- admitted dirty-baseline rollback lanes
- admitted rollback lanes triggered after verification failure or repair

The verified admitted result now consumes that observation surface. Exact
host-call execution preserves an exact result. Ordering-only and
normalized-equivalent outcomes are still bounded and admitted as normalized
equivalence. Hidden host orchestration, missing inputs, queue or state
mismatch, and out-of-contract outcomes still collapse back into reject or
rollback.

## Guardrails Preserved

This implementation still preserves the earlier settled boundaries:

- resident cell storage stays engine-owned
- resident wiring containers stay engine-owned
- lifetime, mutation-entry, raw mutation, raw document mutation,
  realization, rollback, live apply, primitive execution, and final
  verification identity stay engine-owned
- Calc still executes the primitive host calls, but it no longer owns the
  identity of the retained executor shell on the admitted slice

## Validation Focus

The implementation is meant to prove one bounded point:

- the admitted slice now has an explicit primitive host-call executor plan
  and observation that survive both apply and rollback lanes

That leaves the evidence and closeout phases to answer whether this is
strong enough to mark current-slice ownership complete or whether one hybrid
reason still remains.
