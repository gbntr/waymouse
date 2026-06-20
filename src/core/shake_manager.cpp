#include "core/shake_manager.hpp"
#include "gui/shake_overlay.hpp"
#include "core/pointer_manager.hpp"

#include <algorithm>
#include <QGuiApplication>
#include <QScreen>
#include <QMetaObject>
#include <iostream>

namespace waymouse {

ShakeManager::ShakeManager(PointerManager* pointer_mgr, QObject* parent)
    : QObject(parent)
    , m_config{}
    , m_detector(std::make_unique<ShakeDetector>(ShakeSensitivity::Medium))
    , m_monitor(std::make_unique<RawInputMonitor>())
    , m_pointer_mgr(pointer_mgr)
    , m_available(false)
    , m_running(false)
    , m_cursor_x(0)
    , m_cursor_y(0)
{
    // Initialize cursor position to screen center
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        QRect geo = screen->geometry();
        m_cursor_x.store(geo.center().x());
        m_cursor_y.store(geo.center().y());
    }

    // Wire detector callback — fires on the monitor's worker thread.
    // We need to marshal it to the Qt main thread.
    m_detector->on_shake = [this]() {
        // Called from worker thread. Use QMetaObject to invoke on main thread.
        QMetaObject::invokeMethod(this, "on_shake_detected",
                                  Qt::QueuedConnection);
    };

    // Wire monitor callback — accumulate deltas and forward to detector.
    m_monitor->on_event = [this](const InputEvent& ev) {
        this->on_input_event(ev);
    };

    // Create and initialize the overlay
    m_overlay = std::make_unique<ShakeOverlay>(m_pointer_mgr);
}

ShakeManager::~ShakeManager()
{
    stop();
}

void ShakeManager::set_config(const ShakeConfig& cfg)
{
    bool old_enabled = m_config.enabled;
    m_config = normalize_shake_config(cfg);

    // Map sensitivity string to enum
    ShakeSensitivity sens = ShakeSensitivity::Medium;
    if (m_config.sensitivity == "low")
        sens = ShakeSensitivity::Low;
    else if (m_config.sensitivity == "high")
        sens = ShakeSensitivity::High;

    m_detector->set_sensitivity(sens);

    // Restart monitoring if enabled state changed
    if (m_config.enabled != old_enabled)
        update_state();

    emit config_changed();
}

ShakeConfig ShakeManager::config() const
{
    return m_config;
}

bool ShakeManager::is_available() const
{
    return m_available.load();
}

bool ShakeManager::is_running() const
{
    return m_running.load();
}

void ShakeManager::start()
{
    // Initialize the overlay (probes Wayland registry)
    m_available.store(m_overlay->initialize());
    emit availability_changed(m_available.load());

    if (!m_available.load())
    {
        std::cerr << "ShakeManager: overlay not available; feature disabled\n";
        return;
    }

    // Overlay available — start monitoring if enabled
    update_state();
}

void ShakeManager::stop()
{
    m_running.store(false);
    m_monitor->stop();
}

void ShakeManager::on_input_event(const InputEvent& ev)
{
    // Accumulate cursor position
    int x = m_cursor_x.load() + ev.rel_x;
    int y = m_cursor_y.load() + ev.rel_y;

    // Clamp to primary screen bounds
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        QRect geo = screen->geometry();
        x = std::clamp(x, geo.left(), geo.right());
        y = std::clamp(y, geo.top(), geo.bottom());
    }

    m_cursor_x.store(x);
    m_cursor_y.store(y);

    // Forward to detector
    ShakeEvent se;
    se.rel_x = ev.rel_x;
    se.rel_y = ev.rel_y;
    se.timestamp_us = ev.timestamp_us;
    m_detector->push_event(se);
}

void ShakeManager::on_shake_detected()
{
    // This is now on the Qt main thread (via QueuedConnection)
    // Show the overlay at the estimated cursor position
    if (m_overlay && m_overlay->is_available())
    {
        int x = m_cursor_x.load();
        int y = m_cursor_y.load();
        double scale = m_config.scale;
        double duration = m_config.duration;

        m_overlay->show_at(x, y, scale, duration);
    }

    emit shake_detected(m_cursor_x.load(), m_cursor_y.load());
}

void ShakeManager::update_state()
{
    if (m_config.enabled && m_available.load() && !m_running.load())
    {
        m_monitor->start();
        m_running.store(m_monitor->is_running());
    }
    else if ((!m_config.enabled || !m_available.load()) && m_running.load())
    {
        stop();
    }
}

} // namespace waymouse
