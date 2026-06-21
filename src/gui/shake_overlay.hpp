#pragma once

#include <QWindow>
#include <QBackingStore>
#include <QTimer>
#include <memory>
#include <string>

namespace waymouse {

class PointerManager;

// Transient overlay window that appears when a shake is detected.
// Uses QPainter on a QBackingStore to render the scaled cursor + highlight ring.
// In Mango-first mode, prefers layer-shell semantics when possible and falls
// back to a transparent QWindow with explicit degraded status.
class ShakeOverlay : public QWindow
{
    Q_OBJECT

public:
    explicit ShakeOverlay(PointerManager* pointer_mgr = nullptr,
                          QWindow* parent = nullptr);
    ~ShakeOverlay() override;

    // Check if overlay can be shown. In the MVP, always returns true
    // since we use standard QWindow behavior instead of layer-shell.
    bool initialize();
    bool is_available() const;
    void set_compositor_name(std::string compositor_name);
    std::string backend_name() const;
    std::string last_error() const;

    // Show the overlay at the estimated cursor position.
    // scale: scale factor for the cursor image (e.g., 3.0 = 3x size).
    // duration_sec: how long to show before auto-hiding.
    void show_at(int x, int y, double scale, double duration_sec);

protected:
    void exposeEvent(QExposeEvent* event) override;

private slots:
    void hide_overlay();

private:
    void render();
    void draw_fallback(QPainter& painter, int cx, int cy, int scaled_size);
    void position_on_screen(int x, int y, int overlay_size);

    PointerManager* m_pointer_mgr;
    std::unique_ptr<QBackingStore> m_backing_store;
    QTimer* m_hide_timer;
    std::string m_compositor_name;
    std::string m_backend_name;
    std::string m_last_error;
    bool m_available;
    bool m_visible;
    double m_current_scale;
    int m_target_x;
    int m_target_y;
};

} // namespace waymouse
