#pragma once

#include <functional>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <cstdint>

namespace waymouse {

// A single input event from /dev/input/event*.
struct InputEvent
{
    int rel_x;           // relative delta X
    int rel_y;           // relative delta Y
    uint64_t timestamp_us; // monotonic timestamp in microseconds
};

// Monitors /dev/input/event* for mouse REL_X/REL_Y events using epoll.
// Runs an internal worker thread. All callbacks are invoked from that thread.
// Uses libudev to enumerate devices with ID_INPUT_MOUSE=true, no touchpads.
class RawInputMonitor
{
public:
    struct State
    {
        bool running = false;
        bool functional = false;
        bool permission_denied = false;
        bool needs_rescan = true;
        std::string last_error;
    };

    RawInputMonitor();
    ~RawInputMonitor();

    // Start the monitoring thread. Scans udev for mouse devices.
    void start();

    // Stop the thread and close all device fds.
    void stop();

    // True when the thread is actively polling.
    bool is_running() const;

    // Called from the worker thread for each REL_X/REL_Y event.
    std::function<void(const InputEvent&)> on_event;

    // Called when the monitor state changes (start, rescan, permission failure,
    // hotplug recovery, stop). Invoked from the worker thread.
    std::function<void(const State&)> on_state_change;

    bool has_functional_input() const;
    bool permission_denied() const;
    std::string last_error() const;
    State state() const;

private:
    void scan_and_open();
    void run();
    void publish_state();

    std::vector<int> m_fds;     // open /dev/input/event* file descriptors
    std::thread m_thread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_functional;
    std::atomic<bool> m_permission_denied;
    std::atomic<bool> m_needs_rescan;
    mutable std::string m_last_error;
};

} // namespace waymouse
