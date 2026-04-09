# Computational Substrate Global Named-Range Equivalence Rules

Status: completed equivalence rules note for the global named-range admission
plan

## Purpose

This note defines how Calc and engine-owned proof surfaces are compared for
the bounded global single-area named-range admission question.

It exists so the promotion cycle can distinguish:

- exact agreement that is strong enough for live admission
- normalized-equivalent evidence that is useful diagnostically
- outright reject or rollback outcomes

without depending on Calc-local identity.

## Comparison Surfaces

The bounded global named-range candidate is judged across four surfaces:

- computational shadow
- dependency snapshot and recalc queue
- dependency graph shadow
- execution IR

No admission decision is allowed to rely on one surface while ignoring a
divergence on another.

## Global Named-Range Descriptor Identity

For this plan, descriptor identity is represented only by engine-owned
descriptor data:

- `NamedRangeId`
- `maName`
- `meScope`
- `moScopeSheet`
- `maBaseAddress`
- `maTargetExpression`

This promotion cycle is limited to workbook-global names. Exact global
descriptor identity therefore means:

- the same stable `NamedRangeId`
- `meScope == Global`
- no `moScopeSheet`
- the same base address
- the same resolved target expression text after the structural edit

Descriptor identity must never depend on:

- `ScRangeData*`
- Calc pointer identity
- Calc container position
- token-array identity
- UNO object identity

## Exact Computational Equivalence

The computational surface is exact only when all of the following match:

- cell population
- formula tree and formula track
- broadcaster state
- formula-group state
- sorted named-range descriptors

For this admission cycle, exact named-range agreement means:

- the same global descriptor identity
- the same single-area resolved target
- the same final target expression text, including explicit sheet-prefix
  shape when the source text used one

No live-admission decision may rely on a computational comparison that is less
than exact.

## Exact Queue And Graph Equivalence

Queue and graph comparison remain exact-gate surfaces for promotion.

That means:

- recalc queue comparison must be exact
- dependency graph comparison must be exact for promotable cases
- any queue mismatch or graph mismatch forces rollback or defer

The plan does not admit global named-range cases that are merely
computationally exact while graph or queue surfaces drift.

## Execution-IR Equivalence

Execution IR remains an observational proof surface for structural admission,
but it still matters.

For this plan:

- exact IR agreement strengthens promotion evidence
- normalized-equivalent IR agreement is acceptable only when the normalized
  rule is already frozen by the existing IR comparison contract
- IR mismatch on the bounded scalar slice counts as proof failure

## Allowed Normalized-Equivalent Evidence

Normalized-equivalent evidence is allowed only for surfaces that already have
an explicit normalized comparison mode and only when it does not hide
named-range semantic drift.

Examples that may still count as normalized-equivalent evidence:

- formula-tree or formula-track order that already falls under the existing
  normalized graph rules
- existing IR normalization cases that preserve the same reference-update
  meaning

Examples that do not count as normalized-equivalent for this plan:

- different named-range target expression text
- different resolved single-area target
- different descriptor scope
- different explicit-sheet-prefix preservation
- any queue difference

If a case needs target-expression normalization to look successful, it is not
ready for live admission.

## Formula-To-Name And Name-To-Target Representation

The proof cycle does not rely on descriptor text alone.

The relationships are represented as follows:

- descriptor identity and target-expression agreement are checked on the
  computational shadow
- formula-to-name and name-to-target consequences are checked through the
  dependency snapshot and recalc queue built from the after-facade
- listener, broadcaster, formula-tree, and formula-track consequences are
  checked through the dependency graph shadow
- reference-update consequences inside formulas are checked through execution
  IR comparison

The graph shadow stays a listener/broadcaster projection. This plan does not
introduce a new named-range node model.

## Rejected Outcomes

A case is rejected for comparison purposes when any of the following occur:

- workbook-global name ambiguity after case-folding
- local or mixed-scope name resolution
- multi-area target rewriting
- invalid target expression after the structural edit
- queue comparison is not exact
- computational comparison is not exact
- graph comparison is not exact on a promotable case
- execution IR comparison is a mismatch on the bounded scalar slice

## Working Rule For The Admission Pilot

The live-candidate proof cycle should therefore use this order:

1. reject out-of-contract name classes before prediction
2. require exact descriptor and target-expression agreement for any
   promotable case
3. allow normalized-equivalent evidence only where the current graph or IR
   contracts already define it explicitly
4. treat any hidden Calc-local identity dependency as proof failure

This keeps the promotion question aligned with the current exact-verification
rollout standard instead of weakening it for named-range cases.
