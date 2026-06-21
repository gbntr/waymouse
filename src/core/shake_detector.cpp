#include "core/shake_detector.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace waymouse {

static constexpr uint64_t US_PER_MS = 1000;
static constexpr int BASE_COOLDOWN_MS = 500;

ShakeDetector::ShakeDetector(ShakeSensitivity sensitivity)
    : m_sensitivity(sensitivity)
    , m_thresholds(thresholds_for(sensitivity))
    , m_last_trigger_us(0)
{
}

ShakeDetector::~ShakeDetector() = default;

void ShakeDetector::push_event(const ShakeEvent& event)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Discard events with zero or negative timestamps (monotonicity violation)
    if (m_buffer.empty() && event.timestamp_us <= 0)
        return;
    if (!m_buffer.empty() && event.timestamp_us <= m_buffer.back().timestamp_us)
        return;

    m_buffer.push_back(event);

    // Prune events older than 2× the detection window
    prune_old_events(event.timestamp_us - (2 * m_thresholds.time_window_us));

    // Check cooldown
    if (m_last_trigger_us > 0)
    {
        uint64_t elapsed = event.timestamp_us - m_last_trigger_us;
        if (elapsed < m_thresholds.cooldown_us)
        {
            return; // still in cooldown
        }
        m_last_trigger_us = 0; // cooldown passed; reset for next trigger
    }

    // Detect shake
    if (detect_shake_in_window())
    {
        m_last_trigger_us = event.timestamp_us;
        if (on_shake)
        {
            on_shake();
        }
    }
}

void ShakeDetector::set_sensitivity(ShakeSensitivity sensitivity)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sensitivity = sensitivity;
    m_thresholds = thresholds_for(sensitivity);
    m_buffer.clear();
    m_last_trigger_us = 0;
}

void ShakeDetector::prune_old_events(uint64_t cutoff)
{
    while (!m_buffer.empty() && m_buffer.front().timestamp_us < cutoff)
    {
        m_buffer.pop_front();
    }
}

bool ShakeDetector::detect_shake_in_window()
{
    if (m_buffer.size() < 3)
        return false;

    // Determine time window: look at events within m_thresholds.time_window_us
    // from the most recent event
    uint64_t window_start = m_buffer.back().timestamp_us - m_thresholds.time_window_us;

    // Collect events within the window and compute cumulative distance + reversals
    int cumulative_dx = 0;
    int cumulative_dy = 0;
    int reversals_x = 0;
    int reversals_y = 0;
    int last_sign_x = 0;
    int last_sign_y = 0;

    for (const auto& ev : m_buffer)
    {
        if (ev.timestamp_us < window_start)
            continue;

        cumulative_dx += std::abs(ev.rel_x);
        cumulative_dy += std::abs(ev.rel_y);

        // Track X-axis reversals
        if (ev.rel_x != 0)
        {
            int sign = (ev.rel_x > 0) ? 1 : -1;
            if (last_sign_x != 0 && sign != last_sign_x)
                reversals_x++;
            last_sign_x = sign;
        }

        // Track Y-axis reversals
        if (ev.rel_y != 0)
        {
            int sign = (ev.rel_y > 0) ? 1 : -1;
            if (last_sign_y != 0 && sign != last_sign_y)
                reversals_y++;
            last_sign_y = sign;
        }
    }

    // Determine dominant axis: whichever has higher cumulative distance
    bool use_x = cumulative_dx >= cumulative_dy;
    int reversals = use_x ? reversals_x : reversals_y;
    int total_distance = use_x ? cumulative_dx : cumulative_dy;

    // Must meet minimum distance to prevent tiny jitters from triggering
    if (total_distance < m_thresholds.min_distance)
        return false;

    // Must meet minimum reversals
    if (reversals < m_thresholds.min_reversals)
        return false;

    return true;
}

ShakeDetector::Thresholds ShakeDetector::thresholds_for(ShakeSensitivity sensitivity)
{
    // Mapped per clarify D-1:
    // Low:    5 reversals, 300ms window, 300px distance, 500ms cooldown
    // Medium: 3 reversals, 400ms window, 150px distance, 500ms cooldown
    // High:   2 reversals, 500ms window,  75px distance, 500ms cooldown
    switch (sensitivity)
    {
    case ShakeSensitivity::Low:
        return {5, 300 * US_PER_MS, 300, BASE_COOLDOWN_MS * US_PER_MS};
    case ShakeSensitivity::Medium:
        return {3, 400 * US_PER_MS, 150, BASE_COOLDOWN_MS * US_PER_MS};
    case ShakeSensitivity::High:
        return {2, 500 * US_PER_MS, 75, BASE_COOLDOWN_MS * US_PER_MS};
    }
    // Default to Medium
    return {3, 400 * US_PER_MS, 150, BASE_COOLDOWN_MS * US_PER_MS};
}

} // namespace waymouse
