# Hierarchy UX Completion Plan (LVRS)

## Status
- Started: 2026-03-15
- Mode: incremental, contract-first

## Stage 1. Interaction semantics (started)
- Add `HierarchyDropMode`: before/after/child/root
- Add `HierarchyMoveIntent` contract

## Stage 2. Structural operations (started)
- Add deterministic move application by intent
- Preserve sibling order and prevent cycle

## Stage 3. Visibility engine (started)
- Add visible row projection from nodes + expanded state

## Stage 4. Selection/focus state (started)
- Add `HierarchySelectionState` contract

## Stage 5. Keyboard/inline-edit contract (planned)
- Define action enums and state transitions

## Stage 6. Transaction/undo-redo (started)
- Add command stack for move intents

## Stage 7. Sync/conflict baseline (planned)
- Add version metadata contract and conflict markers

## Stage 8. Performance gate (planned)
- Add baseline perf test targets (10k nodes)

## Stage 9. Quality lock (started)
- Add unit tests for move/visibility/undo paths

## Stage 10. Consumer API + sample (planned)
- Expose controller in backend API
- Add minimal consumer example
