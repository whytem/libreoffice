# Computational Substrate InterpretTail Capability-Cluster Expansion Decision Record

Status: completed closeout for the second live evaluator switchover wave

## Decision Summary

This pass is accepted as a real live authority-transfer expansion.

The engine-owned `InterpretTail` delegation seam is no longer limited to the
literal-only text-parsing family. It now includes a bounded host-backed
capability cluster with explicit fallback:

- widened text parsing:
  - `VALUE`
  - `DATEVALUE`
  - `TIMEVALUE`
  - `NUMBERVALUE`
- bounded workbook-local lookup and index:
  - `MATCH`
  - `XMATCH`
  - `LOOKUP`
  - `VLOOKUP`
  - `HLOOKUP`
  - `INDEX`

## What Changed

The live seam now:

- materializes bounded scalar arguments from literals, single-cell
  references, single-cell global names, single-cell sheet-local names, and
  simple scalar expression trees
- routes the promoted lookup/index cluster through the same
  `ScFormulaCell::InterpretTail` engine-first seam
- projects string results directly into `ScFormulaCell`
- records per-function support, shadow, authoritative, and fallback metrics
- records more precise fallback and mismatch reasons

## Calc Path Reduction

This pass satisfies the plan's "real Calc-path retirement or demotion" bar in
the bounded form accepted by the migration strategy:

- for the promoted text-parsing and lookup/index families, `ScInterpreter`
  is no longer the primary path in `authority` mode
- Calc now acts as explicit fallback for unsupported shapes and host
  surfaces on that bounded live cluster

No Calc file deletion was required for this phase. The accepted retirement
form is bounded engine-first routing on the real AutoCalc `InterpretTail`
path.

## Deferred Boundary

The following remain outside this completed wave:

- `XLOOKUP`
- matrix or spill-returning lookup/index shapes
- multi-cell or slice-valued lookup outputs
- external-reference evaluation
- add-ins, macros, DDE, and document-service-dependent runtime families
- workbook-wide default-on delegation

## Why This Counts As Progress

This pass is not another verifier-only closeout:

- it widened a real production routing seam
- it increased the class of formulas that can bypass `ScInterpreter`
  authoritatively
- it added live Calc proof for workbook-local references, global names,
  sheet-local names, simple scalar expressions, and bounded lookup/index
  reads

## Validation Summary

The validating proof for this pass is summarized in
[COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CAPABILITY_CLUSTER_EXPANSION_EVIDENCE.md).

