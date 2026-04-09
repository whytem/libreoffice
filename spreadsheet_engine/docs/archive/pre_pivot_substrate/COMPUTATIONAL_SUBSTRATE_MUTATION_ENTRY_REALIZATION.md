# Computational Substrate Mutation Entry Realization

Status: implemented

## Purpose

This note records the Calc-side apply and realization path used by the
mutation-entry pilot from
[COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md).

The implemented compat surface is:

- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)

## Realization Model

The mutation-entry wrapper keeps the engine authoritative for admitted
mutation intent and admitted after-state while Calc remains the temporary:

- raw mutation host
- live object realization host
- exact verification host
- final rollback host

The realized flow is:

1. capture the admitted before-state shadows, IR shadow, and formula-state
   snapshot
2. raw-apply the admitted mutation request into Calc
3. observe the Calc after-state and build the engine mutation-entry
   transition from that admitted request plus observed after-state
4. apply the transition into engine-owned mutable resident state
5. realize engine-owned resident formula-cell lifetime, scalar cell storage,
   and resident wiring back into Calc
6. verify exact queue, computational, graph, and IR equivalence
7. roll back to the captured before-state if verification fails or a
   repair-detected result is reached

## Host Responsibilities Retained In This Cycle

Calc still owns:

- the raw document mutation APIs used to host the admitted request
- live `ScFormulaCell` realization
- live wiring realization
- final rollback into the captured before-state

The engine owns:

- admitted mutation request shape
- mutation-path classification
- admitted after-state decisions
- resident cell storage after-state
- resident formula-cell lifetime after-state
- resident wiring after-state
- graph, queue, and IR after-state

## Verification And Rollback Rules

The implemented wrapper preserves the same exact proof contract already used
by the admitted resident-state pilots:

- queue comparison must remain exact
- computational comparison must remain a full match
- graph comparison must remain fully matched and non-mismatch
- IR comparison must remain non-mismatch and may only normalize where the
  underlying admitted pilot already allows that

Rollback uses the captured engine-owned before-state, not Calc-local
reconstruction, by re-realizing:

- admitted formula-cell lifetime
- admitted scalar cell storage
- admitted resident wiring
- the captured formula-state snapshot

## Runtime Gate

The compat adapter is protected by an explicit gate:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_MUTATION_ENTRY`

This gate is intentionally separate from the admitted narrow-rollout umbrella
because mutation entry is a new proof surface, not an already-admitted part
of the settled rollout boundary.
