#include "gui/shake_panel.hpp"
#include "core/shake_manager.hpp"
#include "core/config_manager.hpp"
#include "core/runtime_status.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QFont>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>

namespace waymouse {

namespace {

int duration_to_slider(double value)
{
    return static_cast<int>(std::lround(value * 2.0));
}

double slider_to_duration(int value)
{
    return static_cast<double>(value) * 0.5;
}

int scale_to_slider(double value)
{
    return static_cast<int>(std::lround(value * 2.0));
}

double slider_to_scale(int value)
{
    return static_cast<double>(value) * 0.5;
}

} // namespace

class ShakePanel::Impl
{
public:
    QGroupBox* group_box = nullptr;
    QCheckBox* enabled_check = nullptr;
    QComboBox* sensitivity_combo = nullptr;
    QDoubleSpinBox* duration_spin = nullptr;
    QSlider* duration_slider = nullptr;
    QDoubleSpinBox* scale_spin = nullptr;
    QSlider* scale_slider = nullptr;
    QLabel* badge_label = nullptr;
    QLabel* runtime_label = nullptr;
};

ShakePanel::ShakePanel(ShakeManager* manager,
                       ConfigManager* cfg_mgr,
                       QWidget* parent)
    : QWidget(parent)
    , m_manager(manager)
    , m_cfg_mgr(cfg_mgr)
    , m_impl(std::make_unique<Impl>())
{
    setupUi();
    refresh();

    auto* runtime_poll_timer = new QTimer(this);
    runtime_poll_timer->setInterval(1000);
    connect(runtime_poll_timer, &QTimer::timeout,
            this, &ShakePanel::onRuntimePollTimeout);
    runtime_poll_timer->start();

    // Connections — auto-apply on every change
    connect(m_impl->enabled_check, &QCheckBox::toggled,
            this, &ShakePanel::onEnabledToggled);

    connect(m_impl->sensitivity_combo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ShakePanel::onSensitivityChanged);

    connect(m_impl->duration_spin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ShakePanel::onDurationChanged);

    connect(m_impl->scale_spin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ShakePanel::onScaleChanged);

    // React to external changes (availability probe, config loading)
    connect(m_manager, &ShakeManager::config_changed,
            this, &ShakePanel::refresh);
    connect(m_manager, &ShakeManager::availability_changed,
            this, &ShakePanel::onAvailabilityChanged);
}

ShakePanel::~ShakePanel() = default;

void ShakePanel::refresh()
{
    // Block signals while updating widgets programmatically
    m_impl->enabled_check->blockSignals(true);
    m_impl->sensitivity_combo->blockSignals(true);
    m_impl->duration_spin->blockSignals(true);
    m_impl->scale_spin->blockSignals(true);
    m_impl->duration_slider->blockSignals(true);
    m_impl->scale_slider->blockSignals(true);

    auto cfg = m_manager->config();

    m_impl->enabled_check->setChecked(cfg.enabled);

    // Map sensitivity string to combo index
    if (cfg.sensitivity == "low")
        m_impl->sensitivity_combo->setCurrentIndex(0);
    else if (cfg.sensitivity == "high")
        m_impl->sensitivity_combo->setCurrentIndex(2);
    else
        m_impl->sensitivity_combo->setCurrentIndex(1); // medium (default)

    m_impl->duration_spin->setValue(cfg.duration);
    m_impl->scale_spin->setValue(cfg.scale);
    m_impl->duration_slider->setValue(duration_to_slider(cfg.duration));
    m_impl->scale_slider->setValue(scale_to_slider(cfg.scale));

    m_impl->enabled_check->blockSignals(false);
    m_impl->sensitivity_combo->blockSignals(false);
    m_impl->duration_spin->blockSignals(false);
    m_impl->scale_spin->blockSignals(false);
    m_impl->duration_slider->blockSignals(false);
    m_impl->scale_slider->blockSignals(false);

    updateBadge();
    updateRuntimeBadge();
}

void ShakePanel::setupUi()
{
    auto* layout = new QVBoxLayout(this);

    // --- Enable toggle ---
    m_impl->enabled_check = new QCheckBox("Enable Shake to Find", this);
    m_impl->enabled_check->setObjectName("shake_enabled_check");
    QFont font = m_impl->enabled_check->font();
    font.setBold(true);
    m_impl->enabled_check->setFont(font);
    layout->addWidget(m_impl->enabled_check);

    // --- Sensitivity ---
    auto* form = new QFormLayout();
    m_impl->sensitivity_combo = new QComboBox(this);
    m_impl->sensitivity_combo->setObjectName("shake_sensitivity_combo");
    m_impl->sensitivity_combo->addItem("Low (requires stronger shake)");
    m_impl->sensitivity_combo->addItem("Medium (default)");
    m_impl->sensitivity_combo->addItem("High (triggers easily)");
    form->addRow("Sensitivity:", m_impl->sensitivity_combo);
    layout->addLayout(form);

    // --- Duration ---
    QLabel* dur_label = new QLabel("Duration (seconds):", this);
    layout->addWidget(dur_label);

    auto* dur_layout = new QHBoxLayout();
    m_impl->duration_slider = new QSlider(Qt::Horizontal, this);
    m_impl->duration_slider->setObjectName("shake_duration_slider");
    // Map 0.5..5.0 with step 0.5 to int 1..10
    m_impl->duration_slider->setRange(1, 10);
    m_impl->duration_slider->setSingleStep(1);
    m_impl->duration_slider->setTickPosition(QSlider::TicksBelow);
    m_impl->duration_slider->setTickInterval(1);
    dur_layout->addWidget(m_impl->duration_slider);

    m_impl->duration_spin = new QDoubleSpinBox(this);
    m_impl->duration_spin->setObjectName("shake_duration_spin");
    m_impl->duration_spin->setRange(0.5, 5.0);
    m_impl->duration_spin->setSingleStep(0.5);
    m_impl->duration_spin->setDecimals(1);
    m_impl->duration_spin->setSuffix(" s");
    dur_layout->addWidget(m_impl->duration_spin);
    layout->addLayout(dur_layout);

    // Sync slider <-> spinbox
    connect(m_impl->duration_slider, &QSlider::valueChanged, this,
            [this](int val) {
                m_impl->duration_spin->blockSignals(true);
                double duration = slider_to_duration(val);
                m_impl->duration_spin->setValue(duration);
                m_impl->duration_spin->blockSignals(false);
                onDurationChanged(duration);
            });
    connect(m_impl->duration_spin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double val) {
                m_impl->duration_slider->blockSignals(true);
                m_impl->duration_slider->setValue(
                    static_cast<int>(val / 0.5));
                m_impl->duration_slider->blockSignals(false);
            });

    // --- Scale ---
    QLabel* scale_label = new QLabel("Scale factor:", this);
    layout->addWidget(scale_label);

    auto* scale_layout = new QHBoxLayout();
    m_impl->scale_slider = new QSlider(Qt::Horizontal, this);
    m_impl->scale_slider->setObjectName("shake_scale_slider");
    // Map 2.0..5.0 with step 0.5 to int 4..10
    m_impl->scale_slider->setRange(4, 10);
    m_impl->scale_slider->setSingleStep(1);
    m_impl->scale_slider->setTickPosition(QSlider::TicksBelow);
    m_impl->scale_slider->setTickInterval(1);
    scale_layout->addWidget(m_impl->scale_slider);

    m_impl->scale_spin = new QDoubleSpinBox(this);
    m_impl->scale_spin->setObjectName("shake_scale_spin");
    m_impl->scale_spin->setRange(2.0, 5.0);
    m_impl->scale_spin->setSingleStep(0.5);
    m_impl->scale_spin->setDecimals(1);
    m_impl->scale_spin->setSuffix("x");
    scale_layout->addWidget(m_impl->scale_spin);
    layout->addLayout(scale_layout);

    // Sync slider <-> spinbox
    connect(m_impl->scale_slider, &QSlider::valueChanged, this,
            [this](int val) {
                m_impl->scale_spin->blockSignals(true);
                double scale = slider_to_scale(val);
                m_impl->scale_spin->setValue(scale);
                m_impl->scale_spin->blockSignals(false);
                onScaleChanged(scale);
            });
    connect(m_impl->scale_spin,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double val) {
                m_impl->scale_slider->blockSignals(true);
                m_impl->scale_slider->setValue(
                    static_cast<int>(val / 0.5));
                m_impl->scale_slider->blockSignals(false);
            });

    // --- Badge ---
    m_impl->badge_label = new QLabel(this);
    m_impl->badge_label->setObjectName("shake_badge_label");
    m_impl->badge_label->setAlignment(Qt::AlignCenter);
    m_impl->badge_label->setWordWrap(true);
    m_impl->badge_label->setStyleSheet(
        "QLabel { color: #cc0000; font-weight: bold; "
        "padding: 8px; background: #ffeeee; border-radius: 4px; }");
    layout->addWidget(m_impl->badge_label);

    m_impl->runtime_label = new QLabel(this);
    m_impl->runtime_label->setObjectName("shake_runtime_label");
    m_impl->runtime_label->setWordWrap(true);
    layout->addWidget(m_impl->runtime_label);

    layout->addStretch();
}

void ShakePanel::onEnabledToggled(bool checked)
{
    auto cfg = m_manager->config();
    cfg.enabled = checked;
    m_manager->set_config(cfg);
    saveConfig();
}

void ShakePanel::onSensitivityChanged(int index)
{
    auto cfg = m_manager->config();
    switch (index)
    {
    case 0: cfg.sensitivity = "low";    break;
    case 2: cfg.sensitivity = "high";   break;
    default: cfg.sensitivity = "medium"; break;
    }
    m_manager->set_config(cfg);
    saveConfig();
}

void ShakePanel::onDurationChanged(double value)
{
    auto cfg = m_manager->config();
    cfg.duration = value;
    m_manager->set_config(cfg);
    saveConfig();
}

void ShakePanel::onScaleChanged(double value)
{
    auto cfg = m_manager->config();
    cfg.scale = value;
    m_manager->set_config(cfg);
    saveConfig();
}

void ShakePanel::onAvailabilityChanged(bool available)
{
    updateBadge();

    // Disable controls if overlay is not available
    m_impl->enabled_check->setEnabled(available);
    m_impl->sensitivity_combo->setEnabled(available);
    m_impl->duration_spin->setEnabled(available);
    m_impl->duration_slider->setEnabled(available);
    m_impl->scale_spin->setEnabled(available);
    m_impl->scale_slider->setEnabled(available);

    if (!available)
    {
        m_impl->enabled_check->setChecked(false);
    }

    updateRuntimeBadge();
}

void ShakePanel::updateBadge()
{
    if (m_manager->is_available())
    {
        m_impl->badge_label->hide();
    }
    else
    {
        m_impl->badge_label->setText(
            "Shake to Find is temporarily unavailable in this session.");
        m_impl->badge_label->show();
    }
}

void ShakePanel::updateRuntimeBadge()
{
    const auto status = RuntimeStatusPublisher::read();
    const QString text = QString("Runtime: %1 | input=%2 | overlay=%3 | backend=%4 | compositor=%5%6")
        .arg(status.runtime_active ? "running" : "stopped")
        .arg(QString::fromStdString(to_string(status.input_state)))
        .arg(QString::fromStdString(to_string(status.overlay_state)))
        .arg(QString::fromStdString(to_string(status.overlay_backend)))
        .arg(QString::fromStdString(to_string(status.compositor)))
        .arg(status.last_error.empty() ? QString{} : QString(" | %1").arg(QString::fromStdString(status.last_error)));

    m_impl->runtime_label->setText(text);

    const bool runtime_error = status.input_state == RuntimeInputState::PermissionDenied
        || status.overlay_state == RuntimeOverlayState::Failed;
    const bool runtime_degraded = status.input_state == RuntimeInputState::Degraded
        || status.overlay_state == RuntimeOverlayState::Degraded;

    if (runtime_error)
    {
        m_impl->runtime_label->setStyleSheet(
            "QLabel { color: #9c2b2b; font-weight: bold; }");
    }
    else if (runtime_degraded)
    {
        m_impl->runtime_label->setStyleSheet(
            "QLabel { color: #9c6a2b; font-weight: bold; }");
    }
    else
    {
        m_impl->runtime_label->setStyleSheet("QLabel { color: #2b5a9c; }");
    }
}

void ShakePanel::onRuntimePollTimeout()
{
    updateRuntimeBadge();
}

void ShakePanel::saveConfig()
{
    if (m_cfg_mgr)
    {
        m_cfg_mgr->set_shake(m_manager->config());
        m_cfg_mgr->save();
    }
}

} // namespace waymouse
