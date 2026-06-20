#pragma once

#include "core/shake_config.hpp"
#include "core/shake_detector.hpp"
#include "core/raw_input_monitor.hpp"
#include "core/runtime_status.hpp"
#include <QObject>
#include <memory>
#include <atomic>
#include <string>

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
    void set_compositor_name(std::string compositor_name);
    std::string compositor_name() const;
    std::string overlay_backend_name() const;
    bool permission_denied() const;
    bool functional_input_available() const;
    const ShakeRuntimeStatus& status() const;

signals:
    // Emitted when a shake is detected. Connected to overlay show.
    void shake_detected(int estimated_x, int estimated_y);

    // Emitted when any config value changes.
    void config_changed();

    // Emitted when overlay availability changes (probed at startup).
    void availability_changed(bool available);

    // Emitted when the authoritative runtime status changes.
    void status_changed(const ShakeRuntimeStatus& status);

private slots:
    void on_shake_detected();
    void on_input_state_changed(const RawInputMonitor::State& state);

private:
    void on_input_event(const InputEvent& ev);
    void update_state();
    void publish_status();

    ShakeConfig m_config;
    std::unique_ptr<ShakeDetector> m_detector;
    std::unique_ptr<RawInputMonitor> m_monitor;
    std::unique_ptr<ShakeOverlay> m_overlay;
    PointerManager* m_pointer_mgr;
    std::string m_compositor_name;
    ShakeRuntimeStatus m_status;
    std::atomic<bool> m_available;
    std::atomic<bool> m_running;

    // Estimated cursor position (accumulated deltas, clamped to screen)
    std::atomic<int> m_cursor_x;
    std::atomic<int> m_cursor_y;
};

} // namespace waymouse
