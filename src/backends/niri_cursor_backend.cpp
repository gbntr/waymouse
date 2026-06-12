#include "backends/niri_cursor_backend.hpp"
#include "backends/env_cursor_backend.hpp"

namespace waymouse {

NiriCursorBackend::NiriCursorBackend()
    : m_env_backend(std::make_unique<EnvCursorBackend>())
{
}

NiriCursorBackend::~NiriCursorBackend() = default;

bool NiriCursorBackend::apply_cursor_theme(const std::string& theme, int size)
{
    // MVP: niri does not expose a runtime cursor theme reload protocol.
    // `niri msg` does not support cursor theme reload as of current version.
    // Fallback to EnvCursorBackend for persistence.
    m_env_backend->apply_cursor_theme(theme, size);
    return false;
}

bool NiriCursorBackend::supports_runtime_cursor_change() const
{
    return false;
}

std::string NiriCursorBackend::name() const
{
    return "niri-cursor";
}

} // namespace waymouse
