#include "gui/main_window.hpp"
#include "core/device_manager.hpp"
#include "core/config_manager.hpp"
#include "core/pointer_manager.hpp"
#include "core/shake_manager.hpp"
#include "backends/backend.hpp"
#include "gui/device_panel.hpp"
#include "gui/pointer_panel.hpp"
#include "gui/shake_panel.hpp"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QWidget>

namespace waymouse {

class MainWindow::Impl {
public:
    QTabWidget* tab_widget = nullptr;
    QListWidget* device_list = nullptr;
    DevicePanel* panel = nullptr;
    QPushButton* apply_btn = nullptr;
    QLabel* status = nullptr;
    PointerPanel* pointer_panel = nullptr;
    ShakePanel* shake_panel = nullptr;
};

MainWindow::MainWindow(DeviceManager* dev_mgr,
                       ConfigManager* cfg_mgr,
                       Backend* backend,
                       PointerManager* pointer_mgr,
                       ShakeManager* shake_mgr,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_device_manager(dev_mgr)
    , m_config_manager(cfg_mgr)
    , m_backend(backend)
    , m_pointer_manager(pointer_mgr)
    , m_shake_manager(shake_mgr)
    , m_impl(std::make_unique<Impl>())
{
    setupUi();
    refreshDeviceList();

    // Auto-save pointer config on every change
    connect(m_pointer_manager, &PointerManager::changed, this, [this]() {
        PointerConfig cfg;
        cfg.theme = m_pointer_manager->theme();
        cfg.size = m_pointer_manager->size();
        m_config_manager->set_pointer(cfg);
        m_config_manager->save();
    });
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* top_layout = new QVBoxLayout(central);

    m_impl->status = new QLabel(
        "Compositor: " + QString::fromStdString(m_backend->name()), this);
    top_layout->addWidget(m_impl->status);

    m_impl->tab_widget = new QTabWidget(this);

    // --- Device tab ---
    auto* device_tab = new QWidget(this);
    auto* device_layout = new QVBoxLayout(device_tab);

    m_impl->device_list = new QListWidget(this);
    m_impl->panel = new DevicePanel(this);
    m_impl->apply_btn = new QPushButton("Apply", this);

    device_layout->addWidget(m_impl->device_list);
    device_layout->addWidget(m_impl->panel);
    device_layout->addWidget(m_impl->apply_btn);

    m_impl->tab_widget->addTab(device_tab, "Dispositivo");

    // --- Appearance tab ---
    m_impl->pointer_panel = new PointerPanel(m_pointer_manager, this);
    m_impl->tab_widget->addTab(m_impl->pointer_panel, "Apar\u00eancia");

    // --- Shake tab ---
    if (m_shake_manager)
    {
        m_impl->shake_panel = new ShakePanel(m_shake_manager,
                                             m_config_manager, this);
        m_impl->tab_widget->addTab(m_impl->shake_panel, "Shake");
    }

    top_layout->addWidget(m_impl->tab_widget);

    setCentralWidget(central);
    setWindowTitle("waymouse");
    resize(520, 480);

    connect(m_impl->device_list, &QListWidget::currentRowChanged,
            this, &MainWindow::onDeviceSelected);
    connect(m_impl->apply_btn, &QPushButton::clicked,
            this, &MainWindow::onApplyClicked);
}

void MainWindow::refreshDeviceList()
{
    m_impl->device_list->clear();
    m_devices = m_device_manager->enumerate();
    for (const auto& dev : m_devices)
    {
        QString text = QString::fromStdString(dev.name);
        if (!dev.vendor_id.empty())
            text += " (" + QString::fromStdString(dev.vendor_id)
                  + ":" + QString::fromStdString(dev.product_id) + ")";
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
