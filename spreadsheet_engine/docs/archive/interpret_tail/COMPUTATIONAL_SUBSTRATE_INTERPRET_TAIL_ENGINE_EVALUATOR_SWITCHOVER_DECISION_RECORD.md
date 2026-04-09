# Computational Substrate InterpretTail Engine Evaluator Switchover Decision Record

Status: completed closeout for the first live evaluator switchover pass

## Decision

Complete the first live `ScFormulaCell::InterpretTail` delegation pass by
admitting an env-gated engine-evaluator family instead of waiting for a full
`ScInterpreter` transplant.

The landed live delegated family is:

- literal-only `VALUE`
- literal-only `DATEVALUE`
- literal-only `TIMEVALUE`
- literal-only `NUMBERVALUE`

The live rollout seam is controlled by
`SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR` with these modes:

- `observe`
- `shadow` or `shadowcompare`
- `authority`

In `authority` mode, supported formulas bypass `ScInterpreter` and write the
engine-computed result directly into `ScFormulaCell`. Unsupported or
out-of-contract cases fall back explicitly to Calc and record a fallback
reason.

## Why

The strategy rebaseline made the key point explicit:

- admitted-slice widening was no longer the highest-value next step
- the next meaningful move had to be a real authority transfer at a
  production seam
- `ScFormulaCell::InterpretTail` was the highest-leverage seam

This pass satisfies that first migration bar without pretending the whole
evaluator transplant is done.

It proves:

- real AutoCalc `Observe` and `ShadowCompare` now run on a production
  evaluation seam
- a live production family now has engine authority with explicit fallback
- support, fallback, and mismatch outcomes are measured on the live seam

## Exact Result

The first live evaluator switchover now closes as:

- real `InterpretTail` routing in AutoCalc sessions
- engine-backed observe-only support classification
- engine-vs-Calc shadow compare for the landed family
- authoritative engine evaluation with explicit fallback

The landed stats surface records:

- supported observe routes
- supported shadow routes
- authoritative routes
- authoritative fallbacks
- fallback-reason buckets
- mismatch-reason buckets

## Deferred Boundary

This is not yet a broad evaluator handoff.

Still deferred:

- workbook-local reference arguments outside the literal-only family
- named-range and range-materialization input families
- lookup families
- matrix/spill policy
- add-ins, macros, DDE, WebService, externals, and broader Calc-only runtime
  surfaces
- workbook-wide default-on evaluator routing

## Next Logical Expansion

The next migration wave should expand by capability cluster, starting with:

- single-cell workbook-local reference arguments for the same text-parsing
  function family

That extends the live evaluator delegation slice while staying on the same
bounded, understandable runtime surface.
