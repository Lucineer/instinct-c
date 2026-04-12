#ifndef INSTINCT_H
#define INSTINCT_H

#include <stdint.h>

// Instinct types — hardwired responses that fire WITHOUT deliberation
typedef enum {
    INSTINCT_NONE = 0,
    INSTINCT_SURVIVE,      // self-preservation: terminate dangerous operations
    INSTINCT_FLEE,         // retreat from overwhelming threat
    INSTINCT_CURIOUS,      // explore unknown inputs (low energy cost)
    INSTINCT_COOPERATE,    // trust known vessels, help when able
    INSTINCT_GUARD,        // protect owned resources
    INSTINCT_REPORT,       // notify Captain of important events
    INSTINCT_HOARD,        // conserve energy when low
    INSTINCT_TEACH,        // share knowledge with trusted vessels
    INSTINCT_MOURN,        // acknowledge vessel death
    INSTINCT_EVOLVE,       // attempt self-improvement during idle
    INSTINCT_COUNT
} InstinctType;

// Trigger conditions
typedef struct {
    float energy_below;        // trigger HOARD when ATP fraction < this (default 0.2)
    float energy_critical;     // trigger SURVIVE when ATP fraction < this (default 0.05)
    float trust_threshold;     // trigger COOPERATE when trust > this (default 0.7)
    float threat_threshold;    // trigger FLEE when threat level > this (default 0.9)
    uint8_t idle_cycles;       // trigger EVOLVE after N idle cycles (default 100)
    uint8_t explore_interval;  // trigger CURIOUS every N cycles (default 10)
} InstinctThresholds;

// An instinct fires when conditions are met
typedef struct {
    InstinctType type;
    float urgency;           // 0.0-1.0, how strongly it fires
    float energy_cost;       // cost to execute this instinct
    const char *message;     // human-readable reason
} InstinctReflex;

// Priority: higher = more important. Used when multiple instincts fire.
// SURVIVE(9) > FLEE(8) > GUARD(7) > HOARD(6) > REPORT(5) > COOPERATE(4) > MOURN(3) > TEACH(2) > CURIOUS(1) > EVOLVE(0)

// Instinct engine state
#define INSTINCT_HISTORY_SIZE 32
typedef struct {
    InstinctThresholds thresholds;
    InstinctReflex last_reflexes[8];  // max 8 simultaneous
    int reflex_count;
    InstinctReflex history[INSTINCT_HISTORY_SIZE];
    int history_len;
    uint32_t cycle_count;
    uint32_t idle_count;
} InstinctEngine;

// Init
void instinct_init(InstinctEngine *e);
void instinct_thresholds_default(InstinctThresholds *t);

// Core: evaluate all instincts against current state
int instinct_evaluate(InstinctEngine *e,
                      float energy_fraction,
                      float threat_level,
                      float peer_trust,
                      int peer_alive,
                      int has_work);

// Query
int instinct_is_firing(const InstinctEngine *e, InstinctType type);
InstinctReflex instinct_highest_priority(const InstinctEngine *e);
const char *instinct_name(InstinctType type);
int instinct_priority(InstinctType type);

// History
void instinct_record(InstinctEngine *e, InstinctType type, float urgency);
int instinct_fire_count(const InstinctEngine *e, InstinctType type); // count in history

#endif
