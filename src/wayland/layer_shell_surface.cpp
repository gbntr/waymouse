#include "wayland/layer_shell_surface.hpp"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

#include <algorithm>
#include <cstring>
#include <iostream>

namespace waymouse {

// --- File-level listener callbacks (C-compatible) ---

namespace {

struct GlobalData
{
    LayerShellSurface* self;
    wl_display* display;
    bool found;
};

void registry_global(void* data, wl_registry* registry,
                     uint32_t id, const char* interface, uint32_t version)
{
    auto* gd = static_cast<GlobalData*>(data);
    if (std::strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
    {
        gd->self->bind(registry, id, version);
        gd->found = true;
    }
}

void registry_global_remove(void* /*data*/, wl_registry* /*registry*/,
                            uint32_t /*id*/)
{
    // No-op
}

void layer_surface_configure(void* data, zwlr_layer_surface_v1* /*surface*/,
                             uint32_t /*serial*/, uint32_t width, uint32_t height)
{
    auto* self = static_cast<LayerShellSurface*>(data);
    self->on_configure(width, height);
}

void layer_surface_closed(void* data, zwlr_layer_surface_v1* /*surface*/)
{
    auto* self = static_cast<LayerShellSurface*>(data);
    self->on_closed();
}

static const wl_registry_listener kRegistryListener = {
    registry_global,
    registry_global_remove,
};

static const zwlr_layer_surface_v1_listener kLayerSurfaceListener = {
    layer_surface_configure,
    layer_surface_closed,
};

} // anonymous namespace

LayerShellSurface::LayerShellSurface(wl_display* display, wl_surface* qt_surface)
    : m_display(display)
    , m_surface(qt_surface)
    , m_layer_shell(nullptr)
    , m_layer_surface(nullptr)
    , m_valid(false)
    , m_configured(false)
    , m_width(256)
    , m_height(256)
{
    // Roundtrip to discover zwlr_layer_shell_v1
    auto* registry = wl_display_get_registry(m_display);

    GlobalData gd{this, m_display, false};
    wl_registry_add_listener(registry, &kRegistryListener, &gd);
    wl_display_roundtrip(m_display);

    if (!gd.found || !m_valid)
    {
        std::cerr << "LayerShellSurface: zwlr_layer_shell_v1 not available\n";
        wl_registry_destroy(registry);
        m_valid = false;
        return;
    }

    // Second roundtrip for bind completion
    wl_display_roundtrip(m_display);

    // Create layer surface from Qt's wl_surface at overlay layer
    m_layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        m_layer_shell, m_surface, nullptr,
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "waymouse-shake");

    zwlr_layer_surface_v1_add_listener(m_layer_surface,
                                       &kLayerSurfaceListener, this);

    // Input transparency: no keyboard interactivity
    zwlr_layer_surface_v1_set_keyboard_interactivity(m_layer_surface, false);

    // Set initial size and free-floating anchor
    zwlr_layer_surface_v1_set_size(m_layer_surface, m_width, m_height);
    zwlr_layer_surface_v1_set_anchor(m_layer_surface, 0);

    wl_surface_commit(m_surface);
    wl_display_roundtrip(m_display);

    wl_registry_destroy(registry);
}

LayerShellSurface::~LayerShellSurface()
{
    if (m_layer_surface)
    {
        zwlr_layer_surface_v1_destroy(m_layer_surface);
        m_layer_surface = nullptr;
    }
    if (m_layer_shell)
    {
        zwlr_layer_shell_v1_destroy(m_layer_shell);
        m_layer_shell = nullptr;
    }
}

bool LayerShellSurface::is_valid() const
{
    return m_valid && m_configured;
}

void LayerShellSurface::set_size(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    if (m_layer_surface)
        zwlr_layer_surface_v1_set_size(m_layer_surface, width, height);
}

void LayerShellSurface::set_margin(int32_t top, int32_t right,
                                   int32_t bottom, int32_t left)
{
    if (m_layer_surface)
        zwlr_layer_surface_v1_set_margin(m_layer_surface, top, right, bottom, left);
}

void LayerShellSurface::commit()
{
    if (m_surface)
    {
        wl_surface_commit(m_surface);
        wl_display_flush(m_display);
    }
}

void LayerShellSurface::bind(wl_registry* registry, uint32_t id, uint32_t version)
{
    m_layer_shell = static_cast<zwlr_layer_shell_v1*>(
        wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface,
                         std::min(version, 4u)));
    m_valid = (m_layer_shell != nullptr);
}

void LayerShellSurface::on_configure(uint32_t width, uint32_t height)
{
    m_configured = true;
    if (width > 0)
        m_width = width;
    if (height > 0)
        m_height = height;
}

void LayerShellSurface::on_closed()
{
    m_valid = false;
}

} // namespace waymouse
