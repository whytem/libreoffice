# Computational Substrate Named-Range Structural Mapping Rules

Status: complete mapping rules note for the named-range widening plan

## Purpose

This note defines how Calc and engine-owned proof surfaces are compared for
named-range-sensitive structural behavior.

It exists so the widening pilot can distinguish:

- exact agreement
- normalized-equivalent evidence
- outright rejection

without relying on Calc-local identities.

## Comparison Surfaces

Named-range-sensitive structural behavior is evaluated across four surfaces:

- computational shadow
- dependency snapshot and recalc queue
- dependency graph shadow
- execution IR

Each surface plays a different role.

## Named-Range Descriptor Identity

Named-range identity is represented only by engine-owned descriptor data:

- `NamedRangeId`
- `maName`
- `meScope`
- `moScopeSheet`
- `maBaseAddress`
- `maTargetExpression`

Descriptor identity must never depend on:

- `ScRangeData*`
- Calc pointer identity
- Calc container position beyond the stable `NamedRangeId`
- token-array identity
- UNO object identity

Within one facade session, exact identity means the same descriptor survives
comparison after sorting by stable descriptor identity.

## Exact Computational Equivalence

The computational surface is exact only when all of the following match:

- cell population
- formula tree and formula track
- broadcaster state
- formula-group state
- sorted named-range descriptors

For named ranges specifically, exact match means:

- the same descriptor scope
- the same scope sheet, when local
- the same base address
- the same target expression text

No promotion decision may rely on a computational comparison that is less than
exact.

## Normalized Named-Range Equivalence

Normalized equivalence is allowed only as evidence, never as sufficient proof
for live promotion in this plan.

A named-range-sensitive case may be recorded as normalized-equivalent only when
all of the following hold:

- the descriptor still resolves to the same single-area post-edit target
- scope remains unchanged
- formula dependencies derived from the name remain the same after
  normalization
- queue and graph evidence still agree exactly with the observed Calc result

Examples of allowed normalization evidence are:

- spelling differences in a target expression that still normalize to the same
  single-area absolute target
- equivalent formula-tree or formula-track order that already falls under the
  existing normalized graph rules

If target expression normalization would require union rewriting, scope
reinterpretation, or multi-area recovery, the case is rejected rather than
normalized.

## Formula-To-Name And Name-To-Target Representation

Named-range-sensitive structural behavior is not proven by descriptor text
alone.

The proof cycle represents the relationships as follows:

- descriptor identity and target expression are checked on the computational
  surface
- name-to-target and formula-to-name relationships are checked through the
  dependency snapshot and recalc queue built from the after-facade
- listener, broadcaster, formula-tree, and formula-track consequences are
  checked through the dependency graph shadow
- reference-update side effects inside formulas are checked through execution
  IR comparison

The graph shadow remains a listener/broadcaster projection. It does not invent
new named-range node types for this plan.

## Rejected Outcomes

A case is rejected for comparison purposes when any of the following occur:

- named-range scope ambiguity
- multi-area target rewriting
- target expression becomes invalid
- dependency snapshot gains opaque nodes or edges
- queue comparison is not exact
- computational comparison is not exact on a supposedly promotable case
- graph comparison is a mismatch
- execution IR comparison is a mismatch on the admitted scalar slice

## Working Rule For The Pilot

The pilot should therefore use this order:

1. reject out-of-contract named-range classes before prediction
2. require exact descriptor and target-expression agreement for any
   promotable case
3. record normalized-equivalent evidence only for explicitly validation-only
   classes
4. treat any hidden Calc-local identity dependency as a proof failure

This keeps named-range-sensitive structural widening aligned with the current
exact-verification standard instead of weakening it.
