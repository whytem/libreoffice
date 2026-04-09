# Computational Substrate Repair-Sensitive Normalization Plan

Status: completed closeout for the repair-sensitive frontier

## Purpose

This plan focuses only on the repair-sensitive normalization frontier called
out in
[COMPUTATIONAL_SUBSTRATE_MASTER.md](COMPUTATIONAL_SUBSTRATE_MASTER.md).

The goal is not to force an admitted-slice widening where the live host does
not support one. The goal is to close the current repair-sensitive blocker
cleanly by proving which families are:

- already exact and therefore not part of the frontier anymore
- deterministic rollback-only boundaries
- explicit reject-by-rule boundaries
- still real admitted-slice widening targets, if any remain

## Current Boundary

The completed blocker-clearance pass already split repair-sensitive behavior
into:

- deterministic `RepairDetected` rollback
- explicit reject-by-rule
- retained host-only cleanup diagnostics

What it did not settle is whether that taxonomy still hides a real widening
target or whether the repair-sensitive frontier should now be retired as an
active blocker category.

## Working Rule

This pass succeeds only if every currently observed repair-sensitive family
closes as one of:

- `already_exact_and_admitted`
- `deterministic_rollback_boundary`
- `explicit_reject_by_rule`
- `retained_host_only_boundary`
- `real_exact_widening_target`

If no family closes as a real exact widening target, this pass should say so
explicitly and remove repair-sensitive normalization from the top-blocker
list rather than keeping it as a vague future bucket.

## Scope

This plan covers:

- structural same-sheet row and column mutations that currently produce
  `RepairDetected`
- named-range structural repair buckets
- shared-group structural repair buckets
- the remaining repair-sensitive diagnostics that matter to admitted-slice
  decisions

This plan does not cover:

- bounded off-sheet shared-group consumers
- multi-group collapse re-evaluation
- broad storage or listener ownership transfer
- any new widening that depends on hidden host cleanup

## Target Questions

### 1. Exact Structural Families Versus Repair Drift

Question:

- are the current row and column structural repair cases actually missing
  engine normalization
- or are they already exact structural families with only explicit
  post-mutation divergence tests remaining in the repair bucket

### 2. Named-Range Structural Repair

Question:

- does the current named-range structural repair bucket contain a real
  normalization target
- or is it just deterministic rollback on intentionally divergent after-state

### 3. Shared-Group Structural Repair

Question:

- does the current shared-group structural repair bucket contain a real
  normalization target
- or is it likewise a deterministic rollback-only boundary

### 4. Residual Host-Cleanup Diagnostics

Question:

- after the host-shell and same-sheet widening passes, do the retained
  cleanup diagnostics still block any real widening family
- or are they now just explicit retained boundaries rather than a roadmap
  blocker

## Deliverables

This plan is complete only when all of the following exist:

1. one checked-in repair-sensitive matrix that distinguishes exact admitted
   families from deterministic rollback and explicit reject buckets
2. explicit code-level classification for the structural repair buckets so
   they no longer collapse into one generic reason
3. standalone proof for the bounded structural repair buckets
4. live Calc proof for the same repair buckets
5. a closeout decision stating whether repair-sensitive normalization remains
   a real widening blocker
6. updated master, status, and README summaries

## Execution Phases

### Phase 1: Freeze The Repair-Sensitive Matrix

- define the currently active repair-sensitive families precisely
- separate exact structural families from repair-only probes
- record the success rule: explicit closeout is acceptable even without a new
  admitted family

### Phase 2: Make Structural Repair Buckets Explicit

- replace the generic structural repair reason with explicit bucketed reasons
- distinguish plain structural, named-range structural, and shared-group
  structural repair divergence
- keep the behavior rollback-capable and exact

### Phase 3: Prove The Buckets

Add standalone and live proof that:

- the current exact structural families already close exactly
- the current repair-sensitive variants close as deterministic rollback with
  stable reasons
- no hidden host cleanup is being mistaken for an engine-owned widening path

### Phase 4: Close Out The Blocker

- record whether repair-sensitive normalization still contains a real
  widening target
- if not, retire it from the top-blocker list and move the roadmap to
  off-sheet dependency closure
- if a residual target remains, state it narrowly and explicitly

## Success Criteria

This plan counts as complete only if:

- every currently observed repair-sensitive family is placed into an explicit
  exact, rollback, reject, or retained-host bucket
- the structural repair buckets have stable code-level reasons and proof
- the roadmap can say plainly whether repair-sensitive normalization is still
  an active blocker
- replay and diff hygiene remain clean

## Closeout

The plan is complete.

The exact outcome is:

- no new repair-sensitive family was admitted
- the currently observed repair-sensitive structural probes are now explicit
  deterministic rollback buckets with stable reasons
- the currently exact structural families remain exact and already admitted
- repair-sensitive normalization is no longer treated as a top-level widening
  blocker on the roadmap

The repair-sensitive surface remains a real boundary, but it is now a
settled rollback-or-reject boundary rather than the next active widening
program. The roadmap should move to bounded off-sheet dependency closure.
