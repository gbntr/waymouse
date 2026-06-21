#pragma once

#include "core/config_watcher.hpp"
#include "core/mango_ipc_client.hpp"
#include "core/runtime_lock.hpp"
#include "core/runtime_status.hpp"

#include <QObject>
#include <memory>

namespace waymouse {

class ConfigManager;
class PointerManager;
class ShakeManager;

class ShakeSessionRuntime : public QObject
{
    Q_OBJECT

public:
    ShakeSessionRuntime(ConfigManager* config_manager,
                        PointerManager* pointer_manager,
                        std::string compositor_name,
                        QObject* parent = nullptr);
    ~ShakeSessionRuntime() override;

    bool start();
    void stop();

    bool is_running() const;
    const ShakeRuntimeStatus& status() const;
    const std::string& error() const;

signals:
    void status_changed(const ShakeRuntimeStatus& status);

private slots:
    void on_config_changed(const ShakeConfig& cfg);
    void on_manager_status_changed();

private:
    void publish_status();
    void update_status_from_manager(const std::string& error = {});
    void set_error(std::string error, RuntimeInputState input_state = RuntimeInputState::Degraded,
                   RuntimeOverlayState overlay_state = RuntimeOverlayState::Degraded);

    ConfigManager* m_config_manager;
    PointerManager* m_pointer_manager;
    std::string m_compositor_name;
    RuntimeLock m_lock;
    RuntimeStatusPublisher m_publisher;
    ConfigWatcher m_config_watcher;
    MangoIpcClient m_mango_ipc;
    std::unique_ptr<ShakeManager> m_shake_manager;
    ShakeRuntimeStatus m_status;
    bool m_running;
    std::string m_error;
};

} // namespace waymouse
