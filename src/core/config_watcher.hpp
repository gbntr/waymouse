#pragma once

#include "core/shake_config.hpp"

#include <QObject>
#include <QFileSystemWatcher>

namespace waymouse {

class ConfigManager;

class ConfigWatcher : public QObject
{
    Q_OBJECT

public:
    explicit ConfigWatcher(ConfigManager* config_manager, QObject* parent = nullptr);

    bool start();
    void stop();
    bool is_running() const;

signals:
    void shake_config_changed(const ShakeConfig& cfg);

private slots:
    void on_path_changed(const QString& path);
    void on_directory_changed(const QString& path);

private:
    void emit_current_shake();

    ConfigManager* m_config_manager;
    QFileSystemWatcher m_watcher;
    bool m_running;
};

} // namespace waymouse
