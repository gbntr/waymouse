#include "gui/main_window.hpp"
#include "core/device_manager.hpp"
#include "core/config_manager.hpp"
#include "backends/backend.hpp"
#include "gui/device_panel.hpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

namespace waymouse {

class MainWindow::Impl {
public:
    QListWidget* device_list = nullptr;
    DevicePanel* panel = nullptr;
    QPushButton* apply_btn = nullptr;
    QLabel* status = nullptr;
};

MainWindow::MainWindow(DeviceManager* dev_mgr, ConfigManager* cfg_mgr, Backend* backend, QWidget* parent)
    : QMainWindow(parent)
    , m_device_manager(dev_mgr)
    , m_config_manager(cfg_mgr)
    , m_backend(backend)
    , m_impl(std::make_unique<Impl>())
{
    setupUi();
    refreshDeviceList();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    m_impl->device_list = new QListWidget(this);
    m_impl->panel = new DevicePanel(this);
    m_impl->apply_btn = new QPushButton("Apply", this);
    m_impl->status = new QLabel("Compositor: " + QString::fromStdString(m_backend->name()), this);

    layout->addWidget(m_impl->status);
    layout->addWidget(m_impl->device_list);
    layout->addWidget(m_impl->panel);
    layout->addWidget(m_impl->apply_btn);

    setCentralWidget(central);
    setWindowTitle("waymouse");
    resize(480, 400);

    connect(m_impl->device_list, &QListWidget::currentRowChanged, this, &MainWindow::onDeviceSelected);
    connect(m_impl->apply_btn, &QPushButton::clicked, this, &MainWindow::onApplyClicked);
}

void MainWindow::refreshDeviceList()
{
    m_impl->device_list->clear();
    m_devices = m_device_manager->enumerate();
    for (const auto& dev : m_devices)
    {
        QString text = QString::fromStdString(dev.name);
        if (!dev.vendor_id.empty())
            text += " (" + QString::fromStdString(dev.vendor_id) + ":" + QString::fromStdString(dev.product_id) + ")";
        m_impl->device_list->addItem(text);
    }
}

void MainWindow::onDeviceSelected(int index)
{
    if (index < 0 || index >= static_cast<int>(m_devices.size()))
        return;

    const auto& dev = m_devices[index];
    auto cfg = m_config_manager->get(dev.name);
    if (cfg)
        m_impl->panel->setConfig(*cfg);
    else
        m_impl->panel->setConfig(Config{});
}

void MainWindow::onApplyClicked()
{
    int idx = m_impl->device_list->currentRow();
    if (idx < 0 || idx >= static_cast<int>(m_devices.size()))
        return;

    const auto& dev = m_devices[idx];
    Config cfg = m_impl->panel->config();
    m_config_manager->set(dev.name, cfg);
    m_config_manager->save();

    if (m_backend->apply(dev, cfg))
        m_impl->status->setText("Applied to " + QString::fromStdString(dev.name));
    else
        m_impl->status->setText("Failed to apply to " + QString::fromStdString(dev.name));
}

} // namespace waymouse
