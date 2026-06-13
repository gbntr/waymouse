#pragma once

#include <wayland-client.h>

struct zwlr_layer_shell_v1;
struct zwlr_layer_surface_v1;

namespace waymouse {

// Thin C++ wrapper around the generated wlr-layer-shell protocol bindings.
// Creates an overlay surface that sits above all other windows but is
// input-transparent (clicks pass through).
//
// Usage:
//   1. Construct with a connected wl_display and an existing wl_surface
//      (obtained from a QWindow via QPlatformNativeInterface).
//   2. Check is_valid() — if false, the compositor does not support layer-shell.
//   3. Set size and margin, then commit().
class LayerShellSurface
{
public:
    LayerShellSurface(wl_display* display, struct wl_surface* qt_surface);
    ~LayerShellSurface();

    // True if zwlr_layer_shell_v1 is present AND the surface is configured.
    bool is_valid() const;

    // Set the desired surface size in surface coordinates.
    void set_size(uint32_t width, uint32_t height);

    // Set margin from each edge of the screen (for anchored surfaces).
    void set_margin(int32_t top, int32_t right, int32_t bottom, int32_t left);

    // Commit pending state and flush display.
    void commit();

    // Returns true once the compositor has sent a configure event.
    bool is_configured() const { return m_configured; }

    // Called from C-callbacks; must be public for static linkage.
    void bind(wl_registry* registry, uint32_t id, uint32_t version);
    void on_configure(uint32_t width, uint32_t height);
    void on_closed();

private:

    wl_display* m_display;
    struct wl_surface* m_surface;
    zwlr_layer_shell_v1* m_layer_shell;
    zwlr_layer_surface_v1* m_layer_surface;
    bool m_valid;
    bool m_configured;
    uint32_t m_width;
    uint32_t m_height;
};

} // namespace waymouse
