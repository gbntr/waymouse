#include "gui/device_panel.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QCheckBox>

namespace waymouse {

class DevicePanel::Impl {
public:
    QSlider* accel_slider = nullptr;
    QComboBox* profile_combo = nullptr;
    QCheckBox* natural_scroll = nullptr;
    QCheckBox* left_handed = nullptr;
    QLabel* accel_label = nullptr;
};

DevicePanel::DevicePanel(QWidget* parent)
    : QWidget(parent)
    , m_impl(std::make_unique<Impl>())
{
    auto* layout = new QVBoxLayout(this);

    // Acceleration speed
    auto* accel_layout = new QHBoxLayout();
    accel_layout->addWidget(new QLabel("Accel speed:", this));
    m_impl->accel_slider = new QSlider(Qt::Horizontal, this);
    m_impl->accel_slider->setRange(-100, 100); // mapped to -1.0 .. 1.0
    m_impl->accel_slider->setValue(0);
    accel_layout->addWidget(m_impl->accel_slider);
    m_impl->accel_label = new QLabel("0.0", this);
    accel_layout->addWidget(m_impl->accel_label);
    layout->addLayout(accel_layout);

    connect(m_impl->accel_slider, &QSlider::valueChanged, this, [this](int v) {
        m_impl->accel_label->setText(QString::number(v / 100.0, 'f', 2));
    });

    // Profile
    auto* profile_layout = new QHBoxLayout();
    profile_layout->addWidget(new QLabel("Accel profile:", this));
    m_impl->profile_combo = new QComboBox(this);
    m_impl->profile_combo->addItems({"default", "flat", "adaptive"});
    profile_layout->addWidget(m_impl->profile_combo);
    layout->addLayout(profile_layout);

    // Natural scroll
    m_impl->natural_scroll = new QCheckBox("Natural scroll", this);
    layout->addWidget(m_impl->natural_scroll);

    // Left handed
    m_impl->left_handed = new QCheckBox("Left handed", this);
    layout->addWidget(m_impl->left_handed);

    layout->addStretch();
}

void DevicePanel::setConfig(const Config& cfg)
{
    m_impl->accel_slider->setValue(static_cast<int>(cfg.accel_speed * 100.0));
    m_impl->accel_label->setText(QString::number(cfg.accel_speed, 'f', 2));
    m_impl->profile_combo->setCurrentText(QString::fromStdString(cfg.accel_profile));
    m_impl->natural_scroll->setChecked(cfg.natural_scroll);
    m_impl->left_handed->setChecked(cfg.left_handed);
}

Config DevicePanel::config() const
{
    Config cfg;
    cfg.accel_speed = m_impl->accel_slider->value() / 100.0;
    cfg.accel_profile = m_impl->profile_combo->currentText().toStdString();
    cfg.natural_scroll = m_impl->natural_scroll->isChecked();
    cfg.left_handed = m_impl->left_handed->isChecked();
    return cfg;
}

} // namespace waymouse
