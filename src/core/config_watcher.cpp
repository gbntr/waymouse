#include "core/config_watcher.hpp"

#include "core/config_manager.hpp"

#include <QFileInfo>

namespace waymouse {

ConfigWatcher::ConfigWatcher(ConfigManager* config_manager, QObject* parent)
    : QObject(parent)
    , m_config_manager(config_manager)
    , m_running(false)
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged,
            this, &ConfigWatcher::on_path_changed);
}

bool ConfigWatcher::start()
{
    if (m_running || !m_config_manager)
        return false;

    const QString path = QString::fromStdString(m_config_manager->config_path());
    if (QFileInfo::exists(path))
        m_watcher.addPath(path);

    m_running = true;
    emit_current_shake();
    return true;
}

void ConfigWatcher::stop()
{
    if (!m_running)
        return;

    m_watcher.removePaths(m_watcher.files());
    m_running = false;
}

bool ConfigWatcher::is_running() const
{
    return m_running;
}

void ConfigWatcher::on_path_changed(const QString& path)
{
    if (!m_running)
        return;

    if (!path.isEmpty() && QFileInfo::exists(path) && !m_watcher.files().contains(path))
        m_watcher.addPath(path);

    emit_current_shake();
}

void ConfigWatcher::emit_current_shake()
{
    if (!m_config_manager)
        return;

    m_config_manager->load();
    auto cfg = m_config_manager->get_shake().value_or(ShakeConfig{});
    emit shake_config_changed(cfg);
}

} // namespace waymouse
