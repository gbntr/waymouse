#include "core/shake_session_runtime.hpp"

#include "core/config_manager.hpp"
#include "core/pointer_manager.hpp"
#include "core/shake_manager.hpp"

#include <QMetaObject>

#include <utility>

namespace waymouse {

ShakeSessionRuntime::ShakeSessionRuntime(ConfigManager* config_manager,
                                         PointerManager* pointer_manager,
                                         std::string compositor_name,
                                         QObject* parent)
    : QObject(parent)
    , m_config_manager(config_manager)
    , m_pointer_manager(pointer_manager)
    , m_compositor_name(std::move(compositor_name))
    , m_lock()
    , m_publisher()
    , m_config_watcher(config_manager, this)
    , m_shake_manager(std::make_unique<ShakeManager>(pointer_manager))
    , m_status(stopped_runtime_status())
    , m_running(false)
    , m_error()
{
    m_shake_manager->set_compositor_name(m_compositor_name);

    connect(&m_config_watcher, &ConfigWatcher::shake_config_changed,
            this, &ShakeSessionRuntime::on_config_changed);
    connect(m_shake_manager.get(), &ShakeManager::status_changed,
            this, &ShakeSessionRuntime::on_manager_status_changed);
}

ShakeSessionRuntime::~ShakeSessionRuntime()
{
    stop();
}

bool ShakeSessionRuntime::start()
{
    if (m_running)
        return true;

    if (!m_lock.acquire())
    {
        set_error("another shake runtime is already running", RuntimeInputState::Stopped,
                  RuntimeOverlayState::Stopped);
        publish_status();
        return false;
    }

    m_status = stopped_runtime_status();
    m_status.runtime_active = true;
    m_status.compositor = runtime_compositor_from_string(m_compositor_name);
    m_status.input_state = RuntimeInputState::Starting;
    m_status.overlay_state = RuntimeOverlayState::Stopped;
    m_status.overlay_backend = RuntimeOverlayBackend::None;
    m_status.last_error.clear();
    m_status.updated_at = stopped_runtime_status().updated_at;

    if (m_config_manager)
        m_config_manager->load();

    auto shake_cfg = m_config_manager ? m_config_manager->get_shake().value_or(ShakeConfig{}) : ShakeConfig{};
    m_shake_manager->set_config(shake_cfg);
    m_shake_manager->start();

    if (m_mango_ipc.is_available())
    {
        std::string mango_error;
        (void)m_mango_ipc.query_monitor_layouts(&mango_error);
        if (!mango_error.empty())
            m_error = mango_error;
    }

    if (!m_config_watcher.start())
        m_error = m_error.empty() ? "config watcher did not start" : m_error;

    m_running = true;
    update_status_from_manager(m_error);
    publish_status();
    return true;
}

void ShakeSessionRuntime::stop()
{
    if (!m_running && !m_lock.is_locked())
        return;

    if (m_shake_manager)
        disconnect(m_shake_manager.get(), nullptr, this, nullptr);

    if (m_shake_manager)
        m_shake_manager->stop();
    m_config_watcher.stop();
    m_lock.release();
    m_running = false;
    m_status = stopped_runtime_status();
    m_status.compositor = runtime_compositor_from_string(m_compositor_name);
    m_status.last_error = m_error;
    publish_status();
}

bool ShakeSessionRuntime::is_running() const
{
    return m_running;
}

const ShakeRuntimeStatus& ShakeSessionRuntime::status() const
{
    return m_status;
}

const std::string& ShakeSessionRuntime::error() const
{
    return m_error;
}

void ShakeSessionRuntime::on_config_changed(const ShakeConfig& cfg)
{
    if (!m_shake_manager)
        return;

    m_shake_manager->set_config(cfg);
    update_status_from_manager(m_error);
    publish_status();
}

void ShakeSessionRuntime::on_manager_status_changed()
{
    update_status_from_manager(m_error);
    publish_status();
}

void ShakeSessionRuntime::publish_status()
{
    m_status.updated_at = stopped_runtime_status().updated_at;

    m_publisher.publish(m_status);
    emit status_changed(m_status);
}

void ShakeSessionRuntime::update_status_from_manager(const std::string& error)
{
    if (!m_shake_manager)
        return;

    m_status.runtime_active = m_running;
    m_status.compositor = runtime_compositor_from_string(m_compositor_name);
    m_status.overlay_backend = runtime_overlay_backend_from_string(m_shake_manager->overlay_backend_name());
    if (!m_shake_manager->is_available())
        m_status.overlay_state = RuntimeOverlayState::Failed;
    else if (m_status.overlay_backend == RuntimeOverlayBackend::QWindowFallback)
        m_status.overlay_state = RuntimeOverlayState::Degraded;
    else
        m_status.overlay_state = RuntimeOverlayState::Running;

    if (m_shake_manager->is_running())
    {
        if (m_shake_manager->permission_denied())
            m_status.input_state = RuntimeInputState::PermissionDenied;
        else if (m_shake_manager->functional_input_available())
            m_status.input_state = RuntimeInputState::Running;
        else
            m_status.input_state = RuntimeInputState::Degraded;
    }
    else if (!m_running)
    {
        m_status.input_state = RuntimeInputState::Stopped;
        m_status.overlay_state = RuntimeOverlayState::Stopped;
        m_status.overlay_backend = RuntimeOverlayBackend::None;
    }
    else
    {
        m_status.input_state = RuntimeInputState::Starting;
    }

    m_status.last_error = error;
}

void ShakeSessionRuntime::set_error(std::string error, RuntimeInputState input_state,
                                    RuntimeOverlayState overlay_state)
{
    m_error = std::move(error);
    m_status = stopped_runtime_status();
    m_status.compositor = runtime_compositor_from_string(m_compositor_name);
    m_status.input_state = input_state;
    m_status.overlay_state = overlay_state;
    m_status.last_error = m_error;
}

} // namespace waymouse
