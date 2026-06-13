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

private:
    void scan_and_open();
    void run();
    bool is_mouse_device(const std::string& syspath) const;

    std::vector<int> m_fds;     // open /dev/input/event* file descriptors
    std::thread m_thread;
    std::atomic<bool> m_running;
};

} // namespace waymouse
