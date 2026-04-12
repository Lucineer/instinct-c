# Maintenance Notes — instinct-c

## Design Constraints

- **C11 only** — no extensions, no POSIX, no platform headers beyond `<stdint.h>`, `<stddef.h>`, `<string.h>`
- **No dynamic allocation** — all state lives in the `InstinctEngine` struct passed by the caller
- **No `-lm`** — only integer arithmetic and simple float comparisons; no `pow`, `sin`, `cos`, etc.
- **Deterministic** — same inputs always produce same outputs (no randomness)
- **Bare-metal safe** — no syscall dependencies, no floating-point exceptions expected in normal operation

## Extending

- Add new instincts by appending to the `InstinctType` enum (before `INSTINCT_COUNT`)
- Update `instinct_name()` and `instinct_priority()` switch statements
- Add evaluation logic in `instinct_evaluate()` in `instinct.c`
- `last_reflexes[8]` limits simultaneous reflexes; increase if more instincts can fire at once
- `INSTINCT_HISTORY_SIZE 32` limits ring buffer; adjust for longer memory

## Testing

- `make test` runs 15 tests covering all instincts, priority ordering, history, and simultaneous firing
- Add tests to `test_instinct.c` using the `TEST/CHECK/PASS/FAIL` macros
