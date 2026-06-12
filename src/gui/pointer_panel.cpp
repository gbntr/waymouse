#include "gui/pointer_panel.hpp"
#include "core/pointer_manager.hpp"
#include "core/theme_detector.hpp"
#include <QComboBox>
#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QCursor>
#include <filesystem>
#include <string>

namespace waymouse {

class PointerPanel::Impl
{
public:
    QComboBox* theme_combo = nullptr;
    QSlider* size_slider = nullptr;
    QSpinBox* size_spinbox = nullptr;
    QLabel* preview_label = nullptr;
    QLabel* badge_label = nullptr;
    QPushButton* reset_btn = nullptr;
};

PointerPanel::PointerPanel(PointerManager* manager, QWidget* parent)
    : QWidget(parent)
    , m_manager(manager)
    , m_impl(std::make_unique<Impl>())
{
    setupUi();
    refresh();

    // Auto-apply on widget changes — PointerManager handles persistence
    connect(m_impl->theme_combo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &PointerPanel::onThemeChanged);

    connect(m_impl->size_slider,
            &QSlider::valueChanged,
            this,
            &PointerPanel::onSizeChanged);

    connect(m_impl->reset_btn,
            &QPushButton::clicked,
            this,
            &PointerPanel::onResetClicked);

    // React to external changes (e.g., config loading on startup)
    connect(m_manager, &PointerManager::changed, this, &PointerPanel::refresh);
}

PointerPanel::~PointerPanel() = default;

void PointerPanel::refresh()
{
    // Block signals while updating widgets programmatically
    m_impl->theme_combo->blockSignals(true);
    m_impl->size_slider->blockSignals(true);
    m_impl->size_spinbox->blockSignals(true);

    // Repopulate theme ComboBox
    m_impl->theme_combo->clear();
    auto themes = m_manager->available_themes();
    int selected_index = -1;
    std::string current_theme = m_manager->theme();

    for (size_t i = 0; i < themes.size(); ++i)
    {
        m_impl->theme_combo->addItem(QString::fromStdString(themes[i].name));

        if (themes[i].name == current_theme)
            selected_index = static_cast<int>(i);
    }

    // Handle unavailable saved theme
    if (selected_index == -1 && !current_theme.empty())
    {
        m_impl->theme_combo->addItem(
            QString::fromStdString(current_theme) + QString(" (n\u00e3o dispon\u00edvel)"));
        selected_index = m_impl->theme_combo->count() - 1;

        // Set red color for the unavailable item
        m_impl->theme_combo->setItemData(
            selected_index,
            QColor(Qt::red),
            Qt::ForegroundRole);
    }

    if (selected_index >= 0)
        m_impl->theme_combo->setCurrentIndex(selected_index);

    // Update size widgets
    int current_size = m_manager->size();
    m_impl->size_slider->setValue(current_size);
    m_impl->size_spinbox->setValue(current_size);

    m_impl->size_slider->blockSignals(false);
    m_impl->size_spinbox->blockSignals(false);
    m_impl->theme_combo->blockSignals(false);

    updatePreview();
    updateBadge();
}

void PointerPanel::setupUi()
{
    auto* layout = new QVBoxLayout(this);

    // --- Theme section ---
    layout->addWidget(new QLabel("Cursor theme:", this));
    m_impl->theme_combo = new QComboBox(this);
    m_impl->theme_combo->setMinimumWidth(200);
    layout->addWidget(m_impl->theme_combo);

    // --- Size section ---
    layout->addWidget(new QLabel("Cursor size:", this));
    auto* size_layout = new QHBoxLayout();

    m_impl->size_slider = new QSlider(Qt::Horizontal, this);
    m_impl->size_slider->setRange(16, 64);
    m_impl->size_slider->setSingleStep(2);
    m_impl->size_slider->setTickPosition(QSlider::TicksBelow);
    m_impl->size_slider->setTickInterval(2);
    size_layout->addWidget(m_impl->size_slider);

    m_impl->size_spinbox = new QSpinBox(this);
    m_impl->size_spinbox->setRange(16, 64);
    m_impl->size_spinbox->setSingleStep(2);
    m_impl->size_spinbox->setValue(24);
    size_layout->addWidget(m_impl->size_spinbox);
    layout->addLayout(size_layout);

    // Sync slider and spinbox
    connect(m_impl->size_slider, &QSlider::valueChanged,
            m_impl->size_spinbox, &QSpinBox::setValue);
    connect(m_impl->size_spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
            m_impl->size_slider, &QSlider::setValue);

    // --- Preview ---
    m_impl->preview_label = new QLabel(this);
    m_impl->preview_label->setMinimumSize(64, 64);
    m_impl->preview_label->setAlignment(Qt::AlignCenter);
    m_impl->preview_label->setFrameStyle(QFrame::Box | QFrame::Sunken);
    layout->addWidget(m_impl->preview_label);

    // --- Badge ---
    m_impl->badge_label = new QLabel(this);
    m_impl->badge_label->setAlignment(Qt::AlignCenter);
    m_impl->badge_label->setStyleSheet(
        "QLabel { color: #cc6600; font-weight: bold; padding: 4px; }");
    layout->addWidget(m_impl->badge_label);

    // --- Restore defaults button ---
    m_impl->reset_btn = new QPushButton("Restaurar padr\u00e3o", this);
    layout->addWidget(m_impl->reset_btn);

    layout->addStretch();
}

void PointerPanel::onThemeChanged(int index)
{
    if (index < 0)
        return;

    QString text = m_impl->theme_combo->currentText();

    // Strip "(não disponível)" suffix if present
    if (text.endsWith(" (n\u00e3o dispon\u00edvel)"))
    {
        text = text.left(text.length() - 21); // length of " (não disponível)"
    }

    m_manager->set_theme(text.toStdString());
}

void PointerPanel::onSizeChanged(int value)
{
    m_manager->set_size(value);
}

void PointerPanel::onResetClicked()
{
    m_manager->reset_to_defaults();
}

void PointerPanel::updatePreview()
{
    QPixmap pixmap = loadPixmapFromTheme(m_manager->theme(), m_manager->size());

    if (pixmap.isNull())
    {
        // Fallback: use current system cursor pixmap
        pixmap = QCursor(Qt::ArrowCursor).pixmap();
    }

    if (!pixmap.isNull())
    {
        m_impl->preview_label->setPixmap(
            pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    else
    {
        m_impl->preview_label->setText("No preview");
    }
}

void PointerPanel::updateBadge()
{
    if (m_manager->can_apply_runtime())
    {
        m_impl->badge_label->hide();
    }
    else
    {
        m_impl->badge_label->setText(
            "Requer logout/login para aplicar ao sistema");
        m_impl->badge_label->show();
    }
}

QPixmap PointerPanel::loadPixmapFromTheme(const std::string& theme_name,
                                          int /*size*/) const
{
    if (theme_name.empty())
        return QPixmap();

    // Search in user themes first, then system
    const char* home = std::getenv("HOME");
    std::string user_icons =
        (home ? std::string(home) : std::string("/tmp")) + "/.icons";

    auto try_load = [](const std::string& base, const std::string& theme) -> QPixmap {
        auto path = std::filesystem::path(base) / theme / "cursors" / "default";
        std::error_code ec;
        if (std::filesystem::exists(path, ec) && !ec)
        {
            return QPixmap(QString::fromStdString(path.string()));
        }
        return QPixmap();
    };

    // Try user icons first
    QPixmap pix = try_load(user_icons, theme_name);
    if (!pix.isNull())
        return pix;

    // Try system icons
    pix = try_load("/usr/share/icons", theme_name);
    return pix;
}

} // namespace waymouse
