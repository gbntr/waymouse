#include "gui/shake_overlay.hpp"
#include "core/pointer_manager.hpp"
#include "core/theme_detector.hpp"

#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <QPen>
#include <QBrush>
#include <QCursor>

#include <cmath>
#include <filesystem>

namespace waymouse {

// Constants for the highlight ring
static const QColor kRingColor(0x00, 0xA8, 0xFF); // neon blue
static const QColor kRingBorder(0xFF, 0xFF, 0xFF); // white
static constexpr double kRingOpacity = 0.80;
static constexpr double kRingScale = 1.5;          // ring diameter = 1.5x scaled cursor

ShakeOverlay::ShakeOverlay(PointerManager* pointer_mgr, QWindow* parent)
    : QWindow(parent)
    , m_pointer_mgr(pointer_mgr)
    , m_hide_timer(new QTimer(this))
    , m_available(false)
    , m_visible(false)
    , m_current_scale(3.0)
    , m_target_x(0)
    , m_target_y(0)
{
    // Input transparency: clicks pass through to underlying windows
    setFlags(Qt::WindowTransparentForInput |
             Qt::FramelessWindowHint |
             Qt::WindowStaysOnTopHint |
             Qt::WindowDoesNotAcceptFocus);

    // Transparent background for non-drawn areas
    setSurfaceType(QSurface::RasterSurface);
    QSurfaceFormat fmt;
    fmt.setAlphaBufferSize(8);
    setFormat(fmt);

    // Auto-hide timer
    m_hide_timer->setSingleShot(true);
    connect(m_hide_timer, &QTimer::timeout, this, &ShakeOverlay::hide_overlay);

    // Small default size; will be updated in show_at()
    resize(256, 256);
}

ShakeOverlay::~ShakeOverlay()
{
    hide_overlay();
}

bool ShakeOverlay::initialize()
{
    // MVP: QWindow-based overlay works on all compositors.
    // Future: integrate with wlr-layer-shell for guaranteed overlay rendering.
    m_available = true;
    return m_available;
}

bool ShakeOverlay::is_available() const
{
    return m_available;
}

void ShakeOverlay::show_at(int x, int y, double scale, double duration_sec)
{
    if (!m_available)
        return;

    m_target_x = x;
    m_target_y = y;
    m_current_scale = scale;

    // Determine overlay size: base cursor size * scale * ring factor
    int cursor_size = m_pointer_mgr ? m_pointer_mgr->size() : 24;
    int scaled_size = static_cast<int>(cursor_size * scale);
    int diameter = static_cast<int>(scaled_size * kRingScale);
    int overlay_size = std::max(diameter + 20, 128); // minimum 128px

    position_on_screen(x, y, overlay_size);
    resize(overlay_size, overlay_size);

    // Create backing store
    if (!m_backing_store || m_backing_store->size() != size())
    {
        m_backing_store = std::make_unique<QBackingStore>(this);
        m_backing_store->resize(size());
    }

    // Render content
    render();
    m_visible = true;
    show();

    // Start auto-hide timer
    m_hide_timer->start(static_cast<int>(duration_sec * 1000));
}

void ShakeOverlay::exposeEvent(QExposeEvent* /*event*/)
{
    if (isExposed() && m_visible)
    {
        render();
    }
}

void ShakeOverlay::hide_overlay()
{
    m_visible = false;
    hide();
}

void ShakeOverlay::render()
{
    if (!m_backing_store)
        return;

    QImage image(size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    int cx = width() / 2;
    int cy = height() / 2;

    int cursor_size = m_pointer_mgr ? m_pointer_mgr->size() : 24;
    int scaled_size = static_cast<int>(cursor_size * m_current_scale);
    int diameter = static_cast<int>(scaled_size * kRingScale);

    // Try to load cursor theme pixmap
    QPixmap cursor_pix;
    if (m_pointer_mgr && !m_pointer_mgr->theme().empty())
    {
        const char* home = std::getenv("HOME");
        std::string user_icons = (home ? std::string(home) : "/tmp") + "/.icons";
        std::string sys_icons = "/usr/share/icons";
        std::string theme = m_pointer_mgr->theme();

        auto try_path = [&](const std::string& base) -> bool {
            auto path = std::filesystem::path(base) / theme / "cursors" / "default";
            std::error_code ec;
            if (std::filesystem::exists(path, ec) && !ec)
            {
                cursor_pix = QPixmap(QString::fromStdString(path.string()));
                return !cursor_pix.isNull();
            }
            return false;
        };

        try_path(user_icons) || try_path(sys_icons);
    }

    if (!cursor_pix.isNull())
    {
        // Draw scaled cursor centered
        QPixmap scaled = cursor_pix.scaled(scaled_size, scaled_size,
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
        int px = cx - scaled.width() / 2;
        int py = cy - scaled.height() / 2;
        painter.drawPixmap(px, py, scaled);
    }
    else
    {
        // Fallback visual: blue ring + white crosshair
        draw_fallback(painter, cx, cy, scaled_size);
    }

    // Draw highlight ring (always, for visibility on any background)
    painter.save();
    painter.setOpacity(kRingOpacity);
    QPen ring_pen(kRingBorder, 3.0);
    painter.setPen(ring_pen);
    painter.setBrush(QBrush(kRingColor));
    painter.drawEllipse(QPointF(cx, cy), diameter / 2.0, diameter / 2.0);
    painter.restore();

    painter.end();

    // Commit to backing store
    m_backing_store->beginPaint(QRect(0, 0, width(), height()));
    QPaintDevice* device = m_backing_store->paintDevice();
    QPainter store_painter(device);
    store_painter.drawImage(0, 0, image);
    store_painter.end();
    m_backing_store->endPaint();
    m_backing_store->flush(QRect(0, 0, width(), height()));
}

void ShakeOverlay::draw_fallback(QPainter& painter, int cx, int cy,
                                 int scaled_size)
{
    // Solid blue circle
    painter.save();
    painter.setOpacity(kRingOpacity);
    painter.setPen(QPen(kRingBorder, 2.0));
    painter.setBrush(QBrush(kRingColor));
    painter.drawEllipse(QPointF(cx, cy), scaled_size / 2.0, scaled_size / 2.0);
    painter.restore();

    // White crosshair
    int arm_len = std::max(scaled_size / 2, 8);
    QPen cross_pen(Qt::white, 2.0);
    painter.setPen(cross_pen);
    // Horizontal
    painter.drawLine(cx - arm_len, cy, cx + arm_len, cy);
    // Vertical
    painter.drawLine(cx, cy - arm_len, cx, cy + arm_len);
}

void ShakeOverlay::position_on_screen(int x, int y, int overlay_size)
{
    // Clamp to primary screen bounds
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen)
    {
        setPosition(x - overlay_size / 2, y - overlay_size / 2);
        return;
    }

    QRect geo = screen->geometry();
    int pos_x = x - overlay_size / 2;
    int pos_y = y - overlay_size / 2;

    // Clamp to screen edges
    if (pos_x < geo.left())
        pos_x = geo.left();
    if (pos_y < geo.top())
        pos_y = geo.top();
    if (pos_x + overlay_size > geo.right())
        pos_x = geo.right() - overlay_size;
    if (pos_y + overlay_size > geo.bottom())
        pos_y = geo.bottom() - overlay_size;

    setPosition(pos_x, pos_y);
}

} // namespace waymouse
