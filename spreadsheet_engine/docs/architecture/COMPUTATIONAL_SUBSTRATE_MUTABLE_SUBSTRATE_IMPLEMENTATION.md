# Computational Substrate Mutable Substrate Implementation

Status: completed implementation note

## What Landed

The first storage-and-wiring implementation sweep now has an engine-owned
mutable sidecar state for the admitted slice.

The new mutable state lives in:

- [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/MutableComputationalSubstrate.hxx)
- [MutableComputationalSubstrate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/MutableComputationalSubstrate.hxx)

## State Shape

The mutable sidecar is value-semantic and contains:

- an engine-owned `InMemoryWorkbookFacade`
- the current computational observation state
- the current computational workbook shadow
- the last applied mutation
- an applied-mutation count for differential proof

It does not retain Calc pointers or other hidden host ownership.

## Update Model

Bootstrapping still begins from the existing shadow builders, but after that
the state advances from engine-authored transition outputs:

- authority transitions update the in-memory facade in place and then install
  the engine-produced after-shadow
- lifecycle transitions apply the lifecycle sync actions to the in-memory
  facade and then install the engine-produced after-shadow
- structural transitions install the engine-produced after-shadow as the new
  sidecar baseline

That means the admitted slice no longer needs a rebuild from Calc after every
mutation just to keep the engine-side state current.

## Retained Limits

This mutable sidecar is still bounded to the current contract:

- ordinary scalar formulas only
- admitted scalar lifecycle mutations
- admitted single-sheet row/column structural mutations
- clean baseline only

Named-range-sensitive, shared-group, sheet-level, and broader storage classes
remain outside the mutable-state contract.
