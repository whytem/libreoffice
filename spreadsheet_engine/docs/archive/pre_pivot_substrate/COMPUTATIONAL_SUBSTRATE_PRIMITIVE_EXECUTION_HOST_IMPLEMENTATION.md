# Computational Substrate Primitive Execution Host Implementation

Status: completed implementation note

## Purpose

This note records the landed implementation for
[COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md).

The goal of this step was to make the admitted primitive execution shell more
explicitly engine-authored without widening beyond the admitted slice or
reopening broader `ScDocument` host independence.

## Landed Runtime Surface

The new admitted primitive-execution surface now lives in:

- [ComputationalSubstratePrimitiveExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstratePrimitiveExecution.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)
- [ucalc_dependency_shadow.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_dependency_shadow.cxx)

It adds:

- `AdmittedPrimitiveExecutionPlan`
- `PrimitiveExecutionPlanBuildResult`
- `PrimitiveExecutionObservation`

and uses them inside mutation entry before the retained primitive host shell
completes the admitted path.

## What Changed In Mutation Entry

`MutationEntryResult` now carries two additional admitted-slice surfaces:

- `moPrimitiveExecutionPlan`
- `moPrimitiveExecutionObservation`

The runtime now:

- builds an explicit primitive execution plan from the admitted raw document
  mutation record plus primitive realization or primitive rollback identity
- records whether the admitted lane still requires final verification after
  primitive execution completes
- stores the resulting primitive execution observation explicitly on the
  mutation-entry result after final verification has classified the live
  outcome

This means the retained primitive host shell is no longer only the implicit
result of path-specific raw document mutation, realization, rollback, and
verification bookkeeping. It now consumes one named engine-authored
primitive execution plan first.

## Apply And Rollback Shape

The landed implementation keeps the admitted path explicit on both bounded
branches:

- apply lanes carry raw document mutation, primitive realization, and final
  verification stages
- rollback lanes carry raw document mutation, primitive rollback, and final
  verification stages

The primitive execution plan intentionally sits one level above the settled
raw document mutation and primitive realization or rollback records. It
captures the admitted host-shell stage identity without reopening resident
storage, resident wiring, formula-cell lifetime, or final verification
ownership.

## Test Coverage Added

The landed tests now cover:

- direct classifier coverage for the primitive execution observation kinds
- admitted applied mutation-entry lanes carrying an explicit primitive
  execution plan and exact primitive execution observation
- admitted dirty-baseline rollback lanes carrying an explicit primitive
  execution plan and exact primitive execution observation

This keeps the new primitive execution seam visible in the unit suite
instead of only emerging as a side effect of broader mutation-entry
assertions.

## Boundaries Kept On Purpose

This implementation still leaves these host-owned surfaces in Calc:

- the primitive host calls that execute the low-level admitted document
  mutation, realization, and rollback work
- the broader live document shell outside the admitted slice
- the final decision of whether this primitive execution surface becomes a
  settled boundary shift

So this step narrows the host shell and makes it explicit, but it does not
yet claim broad host independence.
