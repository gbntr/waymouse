#pragma once

#include <QWidget>
#include <memory>

namespace waymouse {

class ShakeManager;
class ConfigManager;

// GUI panel for Shake to Find configuration.
// Provides controls for enable/disable, sensitivity, duration, and scale.
// All changes are immediately applied to ShakeManager and persisted via ConfigManager.
class ShakePanel : public QWidget
{
    Q_OBJECT

public:
    explicit ShakePanel(ShakeManager* manager,
                        ConfigManager* cfg_mgr,
                        QWidget* parent = nullptr);
    ~ShakePanel() override;

public slots:
    void refresh();

private slots:
    void onEnabledToggled(bool checked);
    void onSensitivityChanged(int index);
    void onDurationChanged(double value);
    void onScaleChanged(double value);
    void onAvailabilityChanged(bool available);
    void onRuntimePollTimeout();

private:
    void setupUi();
    void updateBadge();
    void updateRuntimeBadge();
    void saveConfig();

    ShakeManager* m_manager;
    ConfigManager* m_cfg_mgr;

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace waymouse
