#pragma once

#include "core/shake_config.hpp"
#include "core/shake_detector.hpp"
#include "core/raw_input_monitor.hpp"
#include <QObject>
#include <memory>
#include <atomic>

namespace waymouse {

class ShakeOverlay;
class PointerManager;

// Qt bridge between the pure-C++ shake detector/input monitor and the GUI.
// Owns the detector and monitor lifetimes. Emits Qt signals for the GUI.
// Thread safety: on_input_event is called from the worker thread;
// all signal emissions are marshaled to the Qt main thread via queued connections.
class ShakeManager : public QObject
{
    Q_OBJECT

public:
    explicit ShakeManager(PointerManager* pointer_mgr = nullptr,
                          QObject* parent = nullptr);
    ~ShakeManager() override;

    // Apply configuration (restarts detector if needed).
    void set_config(const ShakeConfig& cfg);
    ShakeConfig config() const;

    // Whether the overlay protocol is available at runtime.
    bool is_available() const;

    // Whether the detector is actively monitoring.
    bool is_running() const;

    // Start monitoring (creates thread, opens devices).
    // Only starts if enabled=true and overlay is available.
    void start();

    // Stop monitoring (joins thread, closes devices).
    void stop();

    // Access to the overlay for showing/hiding.
    ShakeOverlay* overlay() { return m_overlay.get(); }

signals:
    // Emitted when a shake is detected. Connected to overlay show.
    void shake_detected(int estimated_x, int estimated_y);

    // Emitted when any config value changes.
    void config_changed();

    // Emitted when overlay availability changes (probed at startup).
    void availability_changed(bool available);

private slots:
    void on_shake_detected();

private:
    void on_input_event(const InputEvent& ev);
    void update_state();

    ShakeConfig m_config;
    std::unique_ptr<ShakeDetector> m_detector;
    std::unique_ptr<RawInputMonitor> m_monitor;
    std::unique_ptr<ShakeOverlay> m_overlay;
    PointerManager* m_pointer_mgr;
    std::atomic<bool> m_available;
    std::atomic<bool> m_running;

    // Estimated cursor position (accumulated deltas, clamped to screen)
    std::atomic<int> m_cursor_x;
    std::atomic<int> m_cursor_y;
};

} // namespace waymouse
