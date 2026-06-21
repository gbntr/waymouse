#include "core/shake_detector.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

using namespace waymouse;

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { tests_run++; std::cout << "  TEST: " << name << "... "; } while(0)
#define PASS() do { tests_passed++; std::cout << "PASSED\n"; } while(0)
#define FAIL(msg) do { std::cout << "FAILED: " << msg << "\n"; } while(0)
#define ASSERT_TRUE(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)
#define ASSERT_FALSE(cond, msg) do { if (cond) { FAIL(msg); return; } } while(0)

static void test_medium_3_reversals_triggers()
{
    TEST("Medium: 3 reversals in 400ms + 150px triggers");

    bool triggered = false;
    ShakeDetector det(ShakeSensitivity::Medium);
    det.on_shake = [&]() { triggered = true; };

    // Feed 6 events in 300ms with 50px per event on X axis (alternating)
    // This gives 5 reversals, 300px cumulative
    uint64_t t = 1000000;
    for (int i = 0; i < 6; ++i)
    {
        ShakeEvent ev;
        ev.rel_x = (i % 2 == 0) ? 50 : -50;
        ev.rel_y = 0;
        ev.timestamp_us = t;
        t += 50000; // 50ms between events → 300ms total
        det.push_event(ev);
    }

    ASSERT_TRUE(triggered, "should have triggered on 3+ reversals with enough distance");
    PASS();
}

static void test_medium_continuous_unidirectional_no_trigger()
{
    TEST("Medium: continuous rightward 1000px does NOT trigger");

    bool triggered = false;
    ShakeDetector det(ShakeSensitivity::Medium);
    det.on_shake = [&]() { triggered = true; };

    uint64_t t = 1000000;
    for (int i = 0; i < 20; ++i)
    {
        ShakeEvent ev;
        ev.rel_x = 50;
        ev.rel_y = 0;
        ev.timestamp_us = t;
        t += 25000; // 25ms → 500ms total
        det.push_event(ev);
    }

    ASSERT_FALSE(triggered, "unidirectional motion should NOT trigger shake");
    PASS();
}

static void test_medium_2_reversals_no_trigger()
{
    TEST("Medium: 2 reversals in 400ms + 150px does NOT trigger");

    bool triggered = false;
    ShakeDetector det(ShakeSensitivity::Medium);
    det.on_shake = [&]() { triggered = true; };

    // 2 reversals = 3 events (right, left, right) — only 2 direction changes
    uint64_t t = 1000000;
    // Move right
    det.push_event({50, 0, t}); t += 100000;
    // Move left (reversal 1)
    det.push_event({-50, 0, t}); t += 100000;
    // Move right (reversal 2)
    det.push_event({50, 0, t});

    ASSERT_FALSE(triggered, "only 2 reversals (3 events) should NOT trigger on Medium");
    PASS();
}

static void test_low_5_reversals_triggers()
{
    TEST("Low: 5 reversals in 300ms + 300px triggers");

    bool triggered = false;
    ShakeDetector det(ShakeSensitivity::Low);
    det.on_shake = [&]() { triggered = true; };

    // Feed 8 events with alternating direction = 7 reversals in 280ms, 400px
    uint64_t t = 1000000;
    for (int i = 0; i < 8; ++i)
    {
        ShakeEvent ev;
        ev.rel_x = (i % 2 == 0) ? 50 : -50;
        ev.rel_y = 0;
        ev.timestamp_us = t;
        t += 40000; // 40ms → 280ms total
        det.push_event(ev);
    }

    ASSERT_TRUE(triggered, "should have triggered on 5+ reversals with 300+ px (Low)");
    PASS();
}

static void test_high_2_reversals_triggers()
{
    TEST("High: 2 reversals in 500ms + 75px triggers");

    bool triggered = false;
    ShakeDetector det(ShakeSensitivity::High);
    det.on_shake = [&]() { triggered = true; };

    // 3 events = 2 reversals, 120px cumulative (25+50+50 = 125? no: 25,50,50 — let me fix)
    uint64_t t = 1000000;
    det.push_event({40, 0, t}); t += 150000;
    det.push_event({-40, 0, t}); t += 150000; // reversal 1
    det.push_event({40, 0, t});                // reversal 2 (but 80px dist where? on X it's 120 cumul.)

    // Need >=75px on dominant axis. X: 40+40+40=120 >= 75 ✓
    // Need >=2 reversals: 2 reversals ✓
    // Need within 500ms: 300ms ✓
    ASSERT_TRUE(triggered, "should have triggered on 2 reversals with 75+ px (High)");
    PASS();
}

static void test_rapid_circular_motion_no_trigger()
{
    TEST("Medium: rapid circular motion does NOT trigger");

    bool triggered = false;
    ShakeDetector det(ShakeSensitivity::Medium);
    det.on_shake = [&]() { triggered = true; };

    // Emulate circular motion: alternating X and Y movements
    // that don't produce enough reversals on a single axis
    uint64_t t = 1000000;
    det.push_event({40, 0, t});   t += 50000;
    det.push_event({0, -40, t});  t += 50000;
    det.push_event({-40, 0, t});  t += 50000;
    det.push_event({0, 40, t});   t += 50000;
    det.push_event({40, 0, t});   t += 50000;
    det.push_event({0, -40, t});  t += 50000;

    // X axis: 40, 0, -40, 0, 40, 0 → 2 reversals, 120px cumulative
    // Y axis: 0, -40, 0, 40, 0, -40 → 2 reversals, 120px cumulative
    // Neither axis reaches 3 reversals (Medium threshold)
    // Total time: 250ms, which is within the 400ms window
    ASSERT_FALSE(triggered, "circular motion should NOT trigger on Medium");
    PASS();
}

static void test_sensitivity_change_at_runtime()
{
    TEST("Runtime sensitivity change updates thresholds");

    ShakeDetector det(ShakeSensitivity::Medium);
    bool triggered_low = false;
    bool triggered_high = false;

    // First try to trigger on Low (needs 5 reversals)
    det.set_sensitivity(ShakeSensitivity::Low);
    det.on_shake = [&]() { triggered_low = true; };

    uint64_t t = 1000000;
    // Feed 3 reversals (Medium threshold, not enough for Low)
    det.push_event({50, 0, t}); t += 100000;
    det.push_event({-50, 0, t}); t += 100000;
    det.push_event({50, 0, t}); t += 100000;
    det.push_event({-50, 0, t});

    ASSERT_FALSE(triggered_low, "3 reversals should NOT trigger on Low");

    // Now change to High (needs only 2 reversals, 75px)
    det.set_sensitivity(ShakeSensitivity::High);
    det.on_shake = [&]() { triggered_high = true; };

    t += 1000000; // fresh timestamp
    det.push_event({40, 0, t}); t += 100000;
    det.push_event({-40, 0, t}); t += 100000;
    det.push_event({40, 0, t});

    ASSERT_TRUE(triggered_high, "2 reversals + 120px should trigger on High");
    PASS();
}

static void test_cooldown_prevents_immediate_retrigger()
{
    TEST("Cooldown: shake within 500ms does not re-trigger");

    int trigger_count = 0;
    ShakeDetector det(ShakeSensitivity::High); // easiest to trigger
    det.on_shake = [&]() { trigger_count++; };

    // First shake: 3 events with 2 reversals (triggers on High)
    uint64_t t = 1000000;
    det.push_event({40, 0, t}); t += 100000;
    det.push_event({-40, 0, t}); t += 100000;
    det.push_event({40, 0, t});

    ASSERT_TRUE(trigger_count == 1, "should trigger once");

    // Second shake immediately after (same timestamp pattern, but 100ms later)
    t += 100000; // 100ms after the trigger — still within 500ms cooldown
    det.push_event({40, 0, t}); t += 100000;
    det.push_event({-40, 0, t}); t += 100000;
    det.push_event({40, 0, t});

    ASSERT_TRUE(trigger_count == 1, "should NOT trigger again within cooldown");

    // Third shake after cooldown (600ms later)
    t += 600000; // 600ms after trigger
    det.push_event({40, 0, t}); t += 100000;
    det.push_event({-40, 0, t}); t += 100000;
    det.push_event({40, 0, t});

    ASSERT_TRUE(trigger_count == 2, "should trigger again after cooldown");
    PASS();
}

int main()
{
    std::cout << "=== ShakeDetector Tests ===\n";

    test_medium_3_reversals_triggers();
    test_medium_continuous_unidirectional_no_trigger();
    test_medium_2_reversals_no_trigger();
    test_low_5_reversals_triggers();
    test_high_2_reversals_triggers();
    test_rapid_circular_motion_no_trigger();
    test_sensitivity_change_at_runtime();
    test_cooldown_prevents_immediate_retrigger();

    std::cout << "\nResults: " << tests_passed << "/" << tests_run << " passed\n";
    return (tests_passed == tests_run) ? 0 : 1;
}
