# Computational Substrate Blocker Mapping Rules

Status: frozen mapping rules for the four-blocker clearance program

## Purpose

These rules define the canonical comparison and ownership model for the
blocker-clearance program.

## Rule 1: One Authored After-State

Every candidate family must derive queue, computational shadow, graph, and
IR surfaces from one engine-authored after-state.

The host-observed after-state may validate the result, but it must not act
as the hidden topology oracle for admission.

## Rule 2: Explicit Normalization Only

Normalization is allowed only when it is:

- explicit
- deterministic
- documented
- reused by both standalone and live proof

Hidden host cleanup does not count as normalization.

## Rule 3: Off-Sheet Widening Must Stay Fenced

Off-sheet widening is allowed only when the candidate stays bounded by:

- same workbook
- explicitly enumerated consumer sheets
- exact dependency closure
- exact rollback closure

Anything that implicitly widens into workbook-wide authority remains out of
contract.

## Rule 4: Retained Host Shell Must Not Be The Truth Source

The admitted slice may still execute inside Calc, but Calc-host state must
not be the decisive computational source of truth for:

- expected after-state
- expected IR
- rollback target state
- final-verification truth

## Rule 5: Explicit Family Outcome

Every investigated family must close with one explicit mapping:

- `admitted`
- `rejected_by_rule`
- `retained_host_only`

Generic `out_of_contract` is acceptable only before the family has been
investigated in this program.

## Rule 6: Proof Buckets Must Agree

No family is promoted based on one proof bucket alone.

Facade, standalone, live narrow-rollout, mutation-entry, and replay proof
must agree on the same ownership story.
