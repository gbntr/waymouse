#pragma once

#include <QMainWindow>
#include <memory>
#include <vector>

namespace waymouse {

class DeviceManager;
class ConfigManager;
class Backend;
class PointerManager;
class ShakeManager;
struct Device;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(DeviceManager* dev_mgr,
                        ConfigManager* cfg_mgr,
                        Backend* backend,
                        PointerManager* pointer_mgr,
                        ShakeManager* shake_mgr = nullptr,
                        QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onDeviceSelected(int index);
    void onApplyClicked();

private:
    void setupUi();
    void refreshDeviceList();

    DeviceManager* m_device_manager;
    ConfigManager* m_config_manager;
    Backend* m_backend;
    PointerManager* m_pointer_manager;
    ShakeManager* m_shake_manager;
    std::vector<Device> m_devices;

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace waymouse
