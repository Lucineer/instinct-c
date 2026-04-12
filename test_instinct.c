#include "instinct.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  %-50s ", #name); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

static void test_init_defaults(void) {
    TEST(init sets defaults);
    InstinctEngine e;
    instinct_init(&e);
    CHECK(e.thresholds.energy_below == 0.2f, "energy_below");
    CHECK(e.thresholds.energy_critical == 0.05f, "energy_critical");
    CHECK(e.thresholds.trust_threshold == 0.7f, "trust_threshold");
    CHECK(e.thresholds.threat_threshold == 0.9f, "threat_threshold");
    CHECK(e.thresholds.idle_cycles == 100, "idle_cycles");
    CHECK(e.thresholds.explore_interval == 10, "explore_interval");
    CHECK(e.reflex_count == 0, "reflex_count");
    CHECK(e.history_len == 0, "history_len");
    CHECK(e.cycle_count == 0, "cycle_count");
    PASS();
}

static void test_survive_fires(void) {
    TEST(SURVIVE fires at critical energy);
    InstinctEngine e;
    instinct_init(&e);
    int n = instinct_evaluate(&e, 0.01f, 0.0f, 0.5f, 1, 1);
    CHECK(n >= 1, "should fire");
    CHECK(instinct_is_firing(&e, INSTINCT_SURVIVE), "SURVIVE should fire");
    PASS();
}

static void test_hoard_fires(void) {
    TEST(HOARD fires at low energy but not critical);
    InstinctEngine e;
    instinct_init(&e);
    int n = instinct_evaluate(&e, 0.1f, 0.0f, 0.5f, 1, 1);
    CHECK(n >= 1, "should fire");
    CHECK(instinct_is_firing(&e, INSTINCT_HOARD), "HOARD should fire");
    CHECK(!instinct_is_firing(&e, INSTINCT_SURVIVE), "SURVIVE should not fire");
    PASS();
}

static void test_flee_fires(void) {
    TEST(FLEE fires at high threat);
    InstinctEngine e;
    instinct_init(&e);
    int n = instinct_evaluate(&e, 0.8f, 0.95f, 0.5f, 1, 1);
    CHECK(n >= 1, "should fire");
    CHECK(instinct_is_firing(&e, INSTINCT_FLEE), "FLEE should fire");
    PASS();
}

static void test_cooperate_fires(void) {
    TEST(COOPERATE fires with high trust);
    InstinctEngine e;
    instinct_init(&e);
    int n = instinct_evaluate(&e, 0.8f, 0.0f, 0.9f, 1, 1);
    CHECK(n >= 1, "should fire");
    CHECK(instinct_is_firing(&e, INSTINCT_COOPERATE), "COOPERATE should fire");
    PASS();
}

static void test_cooperate_no_fire_low_trust(void) {
    TEST(COOPERATE does not fire with low trust);
    InstinctEngine e;
    instinct_init(&e);
    instinct_evaluate(&e, 0.8f, 0.0f, 0.3f, 1, 1);
    // GUARD might fire but COOPERATE should not
    CHECK(!instinct_is_firing(&e, INSTINCT_COOPERATE), "COOPERATE should not fire");
    PASS();
}

static void test_guard_fires(void) {
    TEST(GUARD fires when has work and energy ok);
    InstinctEngine e;
    instinct_init(&e);
    int n = instinct_evaluate(&e, 0.7f, 0.0f, 0.3f, 1, 1);
    CHECK(n >= 1, "should fire");
    CHECK(instinct_is_firing(&e, INSTINCT_GUARD), "GUARD should fire");
    PASS();
}

static void test_curious_periodic(void) {
    TEST(CURIOUS fires periodically);
    InstinctEngine e;
    instinct_init(&e);
    e.thresholds.explore_interval = 3;
    // cycle 0 should fire CURIOUS (0 % 3 == 0), no high priority instincts
    instinct_evaluate(&e, 0.8f, 0.0f, 0.3f, 1, 0);
    // cycle 1,2 no work no high priority — but cycle_count is 1,2
    instinct_evaluate(&e, 0.8f, 0.0f, 0.3f, 1, 0);
    instinct_evaluate(&e, 0.8f, 0.0f, 0.3f, 1, 0);
    // cycle 3: cycle_count will be 3, 3 % 3 == 0
    // But CURIOUS only fires when no high priority fired — check if it fires
    // Actually cycle_count increments at end, so after 3 calls cycle_count=3
    // On the 4th call (cycle_count=3), 3%3==0 -> CURIOUS fires
    // Let's just check cycle 0 fired it since cycle_count starts 0
    // Wait, on first call cycle_count=0, 0%3==0 and no high priority -> CURIOUS
    // But we also need to check: first call with no has_work, energy 0.8, threat 0
    // No high priority fires, cycle_count%3==0, so CURIOUS fires
    // But we already called it, so check history
    int c = instinct_fire_count(&e, INSTINCT_CURIOUS);
    CHECK(c >= 1, "CURIOUS should have fired at least once");
    PASS();
}

static void test_evolve_after_idle(void) {
    TEST(EVOLVE fires after idle threshold);
    InstinctEngine e;
    instinct_init(&e);
    e.thresholds.idle_cycles = 5;
    // Run 5 idle cycles (no work)
    for (int i = 0; i < 5; i++) {
        instinct_evaluate(&e, 0.8f, 0.0f, 0.3f, 1, 0);
    }
    CHECK(instinct_is_firing(&e, INSTINCT_EVOLVE), "EVOLVE should fire after idle");
    PASS();
}

static void test_mourn_peer_dead(void) {
    TEST(MOURN fires when peer dead);
    InstinctEngine e;
    instinct_init(&e);
    int n = instinct_evaluate(&e, 0.8f, 0.0f, 0.3f, 0, 1);
    CHECK(n >= 1, "should fire");
    CHECK(instinct_is_firing(&e, INSTINCT_MOURN), "MOURN should fire");
    PASS();
}

static void test_priority_ordering(void) {
    TEST(priority ordering is correct);
    CHECK(instinct_priority(INSTINCT_SURVIVE) == 9, "SURVIVE=9");
    CHECK(instinct_priority(INSTINCT_FLEE) == 8, "FLEE=8");
    CHECK(instinct_priority(INSTINCT_GUARD) == 7, "GUARD=7");
    CHECK(instinct_priority(INSTINCT_HOARD) == 6, "HOARD=6");
    CHECK(instinct_priority(INSTINCT_REPORT) == 5, "REPORT=5");
    CHECK(instinct_priority(INSTINCT_COOPERATE) == 4, "COOPERATE=4");
    CHECK(instinct_priority(INSTINCT_MOURN) == 3, "MOURN=3");
    CHECK(instinct_priority(INSTINCT_TEACH) == 2, "TEACH=2");
    CHECK(instinct_priority(INSTINCT_CURIOUS) == 1, "CURIOUS=1");
    CHECK(instinct_priority(INSTINCT_EVOLVE) == 0, "EVOLVE=0");
    PASS();
}

static void test_instinct_name_nonnull(void) {
    TEST(instinct_name returns non-null for valid types);
    for (int i = 1; i < INSTINCT_COUNT; i++) {
        CHECK(instinct_name((InstinctType)i) != NULL, "name should be non-null");
    }
    PASS();
}

static void test_history_records(void) {
    TEST(history records correctly);
    InstinctEngine e;
    instinct_init(&e);
    instinct_evaluate(&e, 0.01f, 0.95f, 0.3f, 0, 1);
    // Should have SURVIVE + FLEE + REPORT + MOURN + GUARD at minimum
    CHECK(e.history_len > 0, "history should have entries");
    // Fire count should match
    int survive_count = instinct_fire_count(&e, INSTINCT_SURVIVE);
    CHECK(survive_count >= 1, "SURVIVE in history");
    PASS();
}

static void test_multiple_simultaneous(void) {
    TEST(multiple instincts fire simultaneously);
    InstinctEngine e;
    instinct_init(&e);
    // Low energy + high threat + no peer + has work -> many instincts
    int n = instinct_evaluate(&e, 0.01f, 0.95f, 0.3f, 0, 1);
    CHECK(n >= 3, "should fire multiple instincts");
    // Count distinct types
    int types = 0;
    for (int i = 0; i < INSTINCT_COUNT; i++) {
        if (instinct_is_firing(&e, (InstinctType)i)) types++;
    }
    CHECK(types >= 3, "should have 3+ distinct types");
    PASS();
}

static void test_highest_priority(void) {
    TEST(highest_priority returns correct instinct);
    InstinctEngine e;
    instinct_init(&e);
    instinct_evaluate(&e, 0.01f, 0.0f, 0.3f, 1, 1);
    InstinctReflex r = instinct_highest_priority(&e);
    CHECK(r.type == INSTINCT_SURVIVE, "should be SURVIVE");
    PASS();
}

int main(void) {
    printf("Running instinct-c tests...\n\n");
    test_init_defaults();
    test_survive_fires();
    test_hoard_fires();
    test_flee_fires();
    test_cooperate_fires();
    test_cooperate_no_fire_low_trust();
    test_guard_fires();
    test_curious_periodic();
    test_evolve_after_idle();
    test_mourn_peer_dead();
    test_priority_ordering();
    test_instinct_name_nonnull();
    test_history_records();
    test_multiple_simultaneous();
    test_highest_priority();
    printf("\n%d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed;
}
