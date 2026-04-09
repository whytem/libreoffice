# Computational Substrate Blocker Contract

Status: frozen contract for the four-blocker clearance program

## Purpose

This document freezes the contract for the aggressive blocker-clearance
program described in
[COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md](COMPUTATIONAL_SUBSTRATE_BLOCKER_CLEARANCE_PLAN.md).

It defines what counts as success for the four biggest blockers and what
remains out of scope even during an ambitious widening pass.

## Blocker Families

The program addresses four blocker families:

1. exact after-state authoring for broader shared-group families
2. repair-sensitive host normalization
3. bounded off-sheet dependency closure
4. retained host-shell boundary reduction

## In Scope

The program may widen only when all proof surfaces close exactly for the
candidate family.

In-scope target families are:

- same-sheet named-range-combined regroup
- same-sheet named-range-combined merge
- same-sheet named-range-combined bounded collapse only if live Calc actually
  performs the collapse
- broader same-sheet non-edge regroup and merge when the engine can author
  the exact after-topology
- bounded repair-sensitive families that can be normalized explicitly
- bounded same-workbook off-sheet shared-group consumers
- host-shell seams directly required to keep the above families exact

## Explicitly Out Of Scope

The program does not authorize:

- broad default-on rollout
- workbook-wide authority transfer
- broad `ScDocument` ownership transfer
- UI, UNO, rendering, persistence, or import/export migration
- hidden host normalization being treated as engine authority

## Admittance Standard

A family is admitted only when all of the following are true:

- the engine authors the decisive after-state itself
- queue, computational shadow, graph, and IR surfaces close exactly or under
  an already-authorized normalized-equivalent rule
- rollback remains exact when the family is exercised through a rolled-back
  path
- live Calc proof agrees with standalone proof
- the promoted replay baseline stays exact

## Retain Or Reject Standard

A family must remain deferred when any of the following are true:

- live Calc does not actually perform the synthetic topology change
- hidden host cleanup is still decisive
- off-sheet widening would implicitly broaden into workbook-wide authority
- host-shell state remains the real source of truth on the admitted path

## Canonical Proof Ladder

Every candidate family in this program must close on the same proof ladder:

1. facade classification proof
2. standalone computational-substrate proof
3. live narrow-rollout lifecycle or authority proof
4. mutation-entry proof when the family is live-admitted
5. replay-baseline proof

## Expected Outcomes

Each family must end in one of three explicit outcomes:

- `admitted`
- `rejected_by_rule`
- `retained_host_only`

There should be no “unclear” or “generic out of contract” closeout once the
family has been investigated in this program.
