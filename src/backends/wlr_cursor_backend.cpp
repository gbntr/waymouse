#include "backends/wlr_cursor_backend.hpp"
#include "backends/env_cursor_backend.hpp"

namespace waymouse {

WlrCursorBackend::WlrCursorBackend()
    : m_env_backend(std::make_unique<EnvCursorBackend>())
{
}

WlrCursorBackend::~WlrCursorBackend() = default;

bool WlrCursorBackend::apply_cursor_theme(const std::string& theme, int size)
{
    // MVP: wlroots does not expose a runtime cursor theme reload protocol.
    // Fallback to EnvCursorBackend for persistence.
    m_env_backend->apply_cursor_theme(theme, size);
    return false;
}

bool WlrCursorBackend::supports_runtime_cursor_change() const
{
    return false;
}

std::string WlrCursorBackend::name() const
{
    return "mango-cursor";
}

} // namespace waymouse
