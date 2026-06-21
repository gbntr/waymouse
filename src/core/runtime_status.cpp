#include "core/runtime_status.hpp"

#include <nlohmann/json.hpp>

#include <QDateTime>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <utility>

namespace waymouse {

using json = nlohmann::json;

namespace {

std::string now_iso8601()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs).toStdString();
}

template <typename Enum>
std::string enum_fallback(Enum)
{
    return "unknown";
}

} // namespace

std::string to_string(RuntimeInputState state)
{
    switch (state)
    {
    case RuntimeInputState::Stopped: return "stopped";
    case RuntimeInputState::Starting: return "starting";
    case RuntimeInputState::Running: return "running";
    case RuntimeInputState::Degraded: return "degraded";
    case RuntimeInputState::PermissionDenied: return "permission_denied";
    }
    return enum_fallback(state);
}

std::string to_string(RuntimeOverlayState state)
{
    switch (state)
    {
    case RuntimeOverlayState::Stopped: return "stopped";
    case RuntimeOverlayState::Running: return "running";
    case RuntimeOverlayState::Degraded: return "degraded";
    case RuntimeOverlayState::Failed: return "failed";
    }
    return enum_fallback(state);
}

std::string to_string(RuntimeOverlayBackend backend)
{
    switch (backend)
    {
    case RuntimeOverlayBackend::None: return "none";
    case RuntimeOverlayBackend::LayerShell: return "layer-shell";
    case RuntimeOverlayBackend::QWindowFallback: return "qwindow-fallback";
    }
    return enum_fallback(backend);
}

std::string to_string(RuntimeCompositor compositor)
{
    switch (compositor)
    {
    case RuntimeCompositor::Unknown: return "unknown";
    case RuntimeCompositor::Mango: return "mango";
    case RuntimeCompositor::Niri: return "niri";
    case RuntimeCompositor::Libinput: return "libinput";
    }
    return enum_fallback(compositor);
}

RuntimeInputState runtime_input_state_from_string(const std::string& value)
{
    if (value == "starting") return RuntimeInputState::Starting;
    if (value == "running") return RuntimeInputState::Running;
    if (value == "degraded") return RuntimeInputState::Degraded;
    if (value == "permission_denied") return RuntimeInputState::PermissionDenied;
    return RuntimeInputState::Stopped;
}

RuntimeOverlayState runtime_overlay_state_from_string(const std::string& value)
{
    if (value == "running") return RuntimeOverlayState::Running;
    if (value == "degraded") return RuntimeOverlayState::Degraded;
    if (value == "failed") return RuntimeOverlayState::Failed;
    return RuntimeOverlayState::Stopped;
}

RuntimeOverlayBackend runtime_overlay_backend_from_string(const std::string& value)
{
    if (value == "layer-shell") return RuntimeOverlayBackend::LayerShell;
    if (value == "qwindow-fallback") return RuntimeOverlayBackend::QWindowFallback;
    return RuntimeOverlayBackend::None;
}

RuntimeCompositor runtime_compositor_from_string(const std::string& value)
{
    if (value == "mango") return RuntimeCompositor::Mango;
    if (value == "niri") return RuntimeCompositor::Niri;
    if (value == "libinput") return RuntimeCompositor::Libinput;
    return RuntimeCompositor::Unknown;
}

RuntimeStatusPublisher::RuntimeStatusPublisher()
    : m_path(default_path())
{
}

RuntimeStatusPublisher::RuntimeStatusPublisher(std::string path)
    : m_path(std::move(path))
{
}

const std::string& RuntimeStatusPublisher::path() const
{
    return m_path;
}

void RuntimeStatusPublisher::set_path(std::string path)
{
    m_path = std::move(path);
}

bool RuntimeStatusPublisher::publish(const ShakeRuntimeStatus& status) const
{
    try
    {
        std::filesystem::path path(m_path);
        std::filesystem::create_directories(path.parent_path());

        json payload = {
            {"runtime_active", status.runtime_active},
            {"input_state", to_string(status.input_state)},
            {"overlay_state", to_string(status.overlay_state)},
            {"overlay_backend", to_string(status.overlay_backend)},
            {"compositor", to_string(status.compositor)},
            {"last_error", status.last_error},
            {"updated_at", status.updated_at.empty() ? now_iso8601() : status.updated_at},
        };

        std::ofstream out(path);
        out << payload.dump(2) << '\n';
        return static_cast<bool>(out);
    }
    catch (...)
    {
        return false;
    }
}

std::string RuntimeStatusPublisher::default_path()
{
    const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    std::filesystem::path base = runtime_dir ? runtime_dir : "/tmp";
    return (base / "waymouse" / "shake-status.json").string();
}

ShakeRuntimeStatus RuntimeStatusPublisher::read()
{
    ShakeRuntimeStatus status = stopped_runtime_status();
    const std::string path = default_path();

    if (!std::filesystem::exists(path))
        return status;

    try
    {
        std::ifstream in(path);
        json payload;
        in >> payload;

        status.runtime_active = payload.value("runtime_active", false);
        status.input_state = runtime_input_state_from_string(payload.value("input_state", "stopped"));
        status.overlay_state = runtime_overlay_state_from_string(payload.value("overlay_state", "stopped"));
        status.overlay_backend = runtime_overlay_backend_from_string(payload.value("overlay_backend", "none"));
        status.compositor = runtime_compositor_from_string(payload.value("compositor", "unknown"));
        status.last_error = payload.value("last_error", std::string{});
        status.updated_at = payload.value("updated_at", std::string{});
        return status;
    }
    catch (...)
    {
        return status;
    }
}

bool RuntimeStatusPublisher::exists()
{
    return std::filesystem::exists(default_path());
}

ShakeRuntimeStatus stopped_runtime_status()
{
    ShakeRuntimeStatus status;
    status.runtime_active = false;
    status.input_state = RuntimeInputState::Stopped;
    status.overlay_state = RuntimeOverlayState::Stopped;
    status.overlay_backend = RuntimeOverlayBackend::None;
    status.compositor = RuntimeCompositor::Unknown;
    status.updated_at = now_iso8601();
    return status;
}

} // namespace waymouse
