# Architecture Docs

This directory holds the active architecture references for
`spreadsheet_engine/`.

## Start Here

- [COMPUTATIONAL_SUBSTRATE_MASTER.md](COMPUTATIONAL_SUBSTRATE_MASTER.md):
  canonical current-state, scope, metrics, and blocker reference
- [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md):
  strategy reset from substrate-first widening to migration-underwriter
  execution
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md):
  rolling live evaluator migration ledger with the active family, current
  metrics, and current next target
- [../PROJECT_STATUS.md](../PROJECT_STATUS.md):
  concise project-wide status snapshot

## Current Model

The active roadmap is now simple:

1. keep the replay guardrail exact
2. expand real `InterpretTail -> engine` delegation
3. use substrate work only when it removes a live evaluator blocker
4. prioritize replay-imported promoted host-surface and shape cleanup before
   any new reach-expansion slice

## Historical Material

Older pass-by-pass migration documents now live in:

- [../archive/interpret_tail/](../archive/interpret_tail/)
- [../archive/pre_pivot_substrate/](../archive/pre_pivot_substrate/)
- [../archive/](../archive/)
- [../extraction-history/](../extraction-history/)

Use the archive only for implementation archaeology or original decision
context.
