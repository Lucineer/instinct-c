#include "instinct.h"
#include <stddef.h>
#include <string.h>

void instinct_thresholds_default(InstinctThresholds *t) {
    t->energy_below       = 0.2f;
    t->energy_critical    = 0.05f;
    t->trust_threshold    = 0.7f;
    t->threat_threshold   = 0.9f;
    t->idle_cycles        = 100;
    t->explore_interval   = 10;
}

void instinct_init(InstinctEngine *e) {
    memset(e, 0, sizeof(*e));
    instinct_thresholds_default(&e->thresholds);
}

const char *instinct_name(InstinctType type) {
    switch (type) {
        case INSTINCT_NONE:      return "NONE";
        case INSTINCT_SURVIVE:   return "SURVIVE";
        case INSTINCT_FLEE:      return "FLEE";
        case INSTINCT_CURIOUS:   return "CURIOUS";
        case INSTINCT_COOPERATE: return "COOPERATE";
        case INSTINCT_GUARD:     return "GUARD";
        case INSTINCT_REPORT:    return "REPORT";
        case INSTINCT_HOARD:     return "HOARD";
        case INSTINCT_TEACH:     return "TEACH";
        case INSTINCT_MOURN:     return "MOURN";
        case INSTINCT_EVOLVE:    return "EVOLVE";
        default:                 return NULL;
    }
}

int instinct_priority(InstinctType type) {
    switch (type) {
        case INSTINCT_SURVIVE:   return 9;
        case INSTINCT_FLEE:      return 8;
        case INSTINCT_GUARD:     return 7;
        case INSTINCT_HOARD:     return 6;
        case INSTINCT_REPORT:    return 5;
        case INSTINCT_COOPERATE: return 4;
        case INSTINCT_MOURN:     return 3;
        case INSTINCT_TEACH:     return 2;
        case INSTINCT_CURIOUS:   return 1;
        case INSTINCT_EVOLVE:    return 0;
        default:                 return -1;
    }
}

void instinct_record(InstinctEngine *e, InstinctType type, float urgency) {
    if (e->history_len >= INSTINCT_HISTORY_SIZE) {
        memmove(e->history, e->history + 1,
                (INSTINCT_HISTORY_SIZE - 1) * sizeof(InstinctReflex));
        e->history_len = INSTINCT_HISTORY_SIZE - 1;
    }
    e->history[e->history_len].type = type;
    e->history[e->history_len].urgency = urgency;
    e->history[e->history_len].energy_cost = 0.0f;
    e->history[e->history_len].message = instinct_name(type);
    e->history_len++;
}

int instinct_fire_count(const InstinctEngine *e, InstinctType type) {
    int count = 0;
    for (int i = 0; i < e->history_len; i++) {
        if (e->history[i].type == type) count++;
    }
    return count;
}

int instinct_is_firing(const InstinctEngine *e, InstinctType type) {
    for (int i = 0; i < e->reflex_count; i++) {
        if (e->last_reflexes[i].type == type) return 1;
    }
    return 0;
}

InstinctReflex instinct_highest_priority(const InstinctEngine *e) {
    InstinctReflex best = {INSTINCT_NONE, 0.0f, 0.0f, NULL};
    for (int i = 0; i < e->reflex_count; i++) {
        if (instinct_priority(e->last_reflexes[i].type) > instinct_priority(best.type)) {
            best = e->last_reflexes[i];
        }
    }
    return best;
}

int instinct_evaluate(InstinctEngine *e,
                      float energy_fraction,
                      float threat_level,
                      float peer_trust,
                      int peer_alive,
                      int has_work) {
    e->reflex_count = 0;
    int any_high_priority = 0;

    // Helper macro
    #define ADD_REFLEX(t, urg, cost, msg) do { \
        if (e->reflex_count < 8) { \
            e->last_reflexes[e->reflex_count].type = (t); \
            e->last_reflexes[e->reflex_count].urgency = (urg); \
            e->last_reflexes[e->reflex_count].energy_cost = (cost); \
            e->last_reflexes[e->reflex_count].message = (msg); \
            e->reflex_count++; \
        } \
    } while(0)

    // SURVIVE: critical energy
    if (energy_fraction < e->thresholds.energy_critical) {
        float urg = 1.0f - (energy_fraction / e->thresholds.energy_critical);
        if (urg > 1.0f) urg = 1.0f;
        ADD_REFLEX(INSTINCT_SURVIVE, urg, 0.1f, "Energy critical");
        if (instinct_priority(INSTINCT_SURVIVE) >= 5) any_high_priority = 1;
    }

    // HOARD: low energy but not critical
    if (energy_fraction < e->thresholds.energy_below &&
        energy_fraction >= e->thresholds.energy_critical) {
        float urg = 1.0f - (energy_fraction / e->thresholds.energy_below);
        ADD_REFLEX(INSTINCT_HOARD, urg, 0.05f, "Energy low");
        if (instinct_priority(INSTINCT_HOARD) >= 5) any_high_priority = 1;
    }

    // FLEE: overwhelming threat
    if (threat_level > e->thresholds.threat_threshold) {
        float urg = (threat_level - e->thresholds.threat_threshold) / (1.0f - e->thresholds.threat_threshold);
        if (urg > 1.0f) urg = 1.0f;
        ADD_REFLEX(INSTINCT_FLEE, urg, 0.3f, "Overwhelming threat");
        if (instinct_priority(INSTINCT_FLEE) >= 5) any_high_priority = 1;
    }

    // COOPERATE: high trust + has work
    if (peer_trust > e->thresholds.trust_threshold && has_work) {
        float urg = (peer_trust - e->thresholds.trust_threshold) / (1.0f - e->thresholds.trust_threshold);
        ADD_REFLEX(INSTINCT_COOPERATE, urg, 0.15f, "Trusted peer needs help");
    }

    // GUARD: has work and energy ok
    if (has_work && energy_fraction > 0.5f) {
        ADD_REFLEX(INSTINCT_GUARD, energy_fraction, 0.2f, "Protecting resources");
        if (instinct_priority(INSTINCT_GUARD) >= 5) any_high_priority = 1;
    }

    // MOURN: peer dead
    if (!peer_alive) {
        ADD_REFLEX(INSTINCT_MOURN, 1.0f, 0.01f, "Peer vessel lost");
    }

    // REPORT: any instinct with priority >= 5 fired
    if (any_high_priority) {
        ADD_REFLEX(INSTINCT_REPORT, 0.8f, 0.02f, "Important event detected");
    }

    // CURIOUS: periodic, only when no higher-priority fired
    if (any_high_priority == 0 &&
        e->cycle_count % e->thresholds.explore_interval == 0) {
        ADD_REFLEX(INSTINCT_CURIOUS, 0.3f, 0.05f, "Exploring environment");
    }

    // EVOLVE: after idle threshold
    if (!has_work) {
        e->idle_count++;
    } else {
        e->idle_count = 0;
    }
    if (e->idle_count >= e->thresholds.idle_cycles) {
        ADD_REFLEX(INSTINCT_EVOLVE, 0.5f, 0.1f, "Idle — self-improving");
        e->idle_count = 0; // reset after firing
    }

    #undef ADD_REFLEX

    // Record all firing instincts to history
    for (int i = 0; i < e->reflex_count; i++) {
        instinct_record(e, e->last_reflexes[i].type, e->last_reflexes[i].urgency);
    }

    e->cycle_count++;
    return e->reflex_count;
}
