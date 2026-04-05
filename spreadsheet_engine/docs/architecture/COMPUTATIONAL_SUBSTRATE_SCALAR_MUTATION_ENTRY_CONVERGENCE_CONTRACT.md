# Computational Substrate Scalar Mutation Entry Convergence Contract

Status: frozen scalar-entry convergence contract

## Purpose

This note freezes the exact proof surface for
[COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md)
before any scalar-entry convergence work proceeds.

The purpose of this contract is to keep the next cycle narrowly focused on
one remaining equivalence gap:

- direct admitted-slice `SetScalarValue`
- queue already exact
- graph already exact and full-match
- IR already accepted
- broadcaster-only computational mismatch after live realization

This is an equivalence cycle, not a new authority grab.

## In-Scope Workbook Slice

The admitted workbook slice remains the already-proven mutation-entry slice:

- clean baseline only
- ordinary scalar formulas only
- no shared groups
- no named-range-sensitive structural behavior
- no off-sheet structural consumers
- no sheet-wide structural edits
- no token-container ownership migration
- no host-only service widening

If the workbook leaves that slice, this cycle must reject, roll back, or
defer instead of silently widening.

## In-Scope Mutation Surface

This convergence cycle is scoped to one mutation class only:

- `SetScalarValue`

The covered scalar-entry families are:

- scalar overwrite of an existing scalar cell with formula dependents
- scalar overwrite that invalidates a direct listener
- scalar overwrite that invalidates a transitive listener chain
- scalar overwrite with multiple admitted listener consumers
- scalar overwrite where broadcaster cleanup or reuse is required after live
  realization

Everything else remains out of contract for this cycle:

- `SetFormula`
- `ClearCell`
- admitted structural-entry classes
- named-range-sensitive scalar consumers
- shared-group-sensitive scalar consumers

Those already have separate proof paths and must not be pulled into this
reassessment.

## Engine-Owned Surface Being Reassessed

This cycle assumes the engine already remains authoritative on the admitted
slice for:

- resident cell storage
- resident wiring containers
- formula-cell lifetime decisions
- mutable computational state
- graph, wiring, and queue decisions
- admitted scalar mutation request shape and routing

The only question being reopened is whether Calc live realization can be
brought into exact broadcaster alignment with those already-engine-owned
after-state decisions.

## Retained Calc-Owned Surfaces

Calc intentionally remains host-owned in this cycle for:

- raw mutation application to the live document
- live object realization from engine-owned resident state
- final rollback after failed verification
- all out-of-contract workbook and mutation classes

This cycle does not claim broad live mutation-entry ownership. It only tries
to remove the last known scalar-entry equivalence caveat on the admitted
slice.

## Exact Success Standard

Scalar-entry convergence counts as success only if all of the following hold:

- queue comparison remains exact
- graph comparison remains exact and full-match
- computational comparison becomes full-match
- broadcaster equivalence holds without hiding mismatch behind relaxed
  normalization
- dirty-baseline rejection remains deterministic
- rollback remains explicit and green on induced divergence

This cycle does not succeed merely because the mismatch looks harmless. It
must become exact.

## Convergence Versus Immediate Defer

This cycle counts as convergence only if the remaining scalar-entry mismatch
can be explained and removed as one of the following:

- pure live realization ordering
- duplicate broadcaster materialization
- empty broadcaster persistence
- listener-anchor canonicalization drift

This cycle must immediately defer again if the remaining mismatch proves to
be any of the following:

- a true dependency-graph error
- a queue divergence
- an admitted-slice state shape that depends on hidden Calc-only identity
- a host-boundary rule that cannot be represented by the resident engine
  substrate
- a fix that requires widening beyond admitted scalar `SetScalarValue`

## Required Closeout Shape

This contract is satisfied only if the closeout ends in one of these three
explicit states:

- admitted scalar mutation entry proceeds into the settled live boundary
- admitted scalar mutation entry remains validation-only with one clearly
  bounded reason
- admitted scalar mutation entry is deferred again because the mismatch is
  deeper than canonicalization

Anything less explicit than that is out of contract.
