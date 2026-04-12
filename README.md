# instinct-c

A pure C11 library implementing a hardwired reflex system for bare-metal agents.

Zero dependencies. No dynamic allocation. No `-lm` needed.

## Instincts

| Instinct | Priority | Trigger |
|----------|----------|---------|
| SURVIVE | 9 | Energy fraction below critical |
| FLEE | 8 | Threat level above threshold |
| GUARD | 7 | Has work and energy > 0.5 |
| HOARD | 6 | Energy below low threshold (but not critical) |
| REPORT | 5 | Any instinct with priority ≥ 5 fires |
| COOPERATE | 4 | Peer trust above threshold and has work |
| MOURN | 3 | Peer vessel dead |
| TEACH | 2 | *(reserved)* |
| CURIOUS | 1 | Periodic exploration (no high-priority active) |
| EVOLVE | 0 | Idle for N cycles |

## Usage

```c
#include "instinct.h"

InstinctEngine engine;
instinct_init(&engine);

// Each tick:
int count = instinct_evaluate(&engine,
    0.75f,   // energy fraction 0.0-1.0
    0.1f,    // threat level 0.0-1.0
    0.8f,    // peer trust 0.0-1.0
    1,       // peer alive
    1        // has work
);

InstinctReflex top = instinct_highest_priority(&engine);
printf("Action: %s (urgency %.2f)\n", instinct_name(top.type), top.urgency);
```

## Building

```sh
make test
```

## Files

- `instinct.h` — Public API
- `instinct.c` — Implementation
- `test_instinct.c` — Test suite (15 tests)
- `Makefile` — Build system

## License

Public domain / CC0.
