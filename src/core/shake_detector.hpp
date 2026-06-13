#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <cstdint>

namespace waymouse {

// A single motion event from the input subsystem.
struct ShakeEvent
{
    int rel_x;           // relative delta X
    int rel_y;           // relative delta Y
    uint64_t timestamp_us; // monotonic timestamp in microseconds
};

// Sensitivity levels for shake detection.
// Mapped to concrete thresholds per clarify D-1.
enum class ShakeSensitivity
{
    Low,    // 5 reversals in 300ms, 300px cumulative
    Medium, // 3 reversals in 400ms, 150px cumulative
    High    // 2 reversals in 500ms, 75px cumulative
};

// Pure C++ algorithm that detects shake gestures from relative motion events.
// Thread-safe: push_event() can be called from any thread.
// The on_shake callback is invoked from the calling thread of push_event().
class ShakeDetector
{
public:
    explicit ShakeDetector(ShakeSensitivity sensitivity = ShakeSensitivity::Medium);
    ~ShakeDetector();

    // Push a motion event. The callback fires synchronously if a shake is detected.
    void push_event(const ShakeEvent& event);

    // Change sensitivity at runtime. Resets internal state.
    void set_sensitivity(ShakeSensitivity sensitivity);

    // Invoked exactly once per detected shake from the calling thread.
    // No duplicate callbacks within the cooldown period (500ms).
    std::function<void()> on_shake;

private:
    struct Thresholds
    {
        int min_reversals;
        uint64_t time_window_us;
        int min_distance;
        uint64_t cooldown_us;
    };

    static Thresholds thresholds_for(ShakeSensitivity sensitivity);
    void prune_old_events(uint64_t cutoff);
    bool detect_shake_in_window();

    ShakeSensitivity m_sensitivity;
    Thresholds m_thresholds;
    std::deque<ShakeEvent> m_buffer;
    uint64_t m_last_trigger_us; // timestamp of last shake trigger (for cooldown)
    std::mutex m_mutex;
};

} // namespace waymouse
