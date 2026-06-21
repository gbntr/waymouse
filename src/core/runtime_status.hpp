#pragma once

#include <string>

namespace waymouse {

enum class RuntimeInputState
{
    Stopped,
    Starting,
    Running,
    Degraded,
    PermissionDenied
};

enum class RuntimeOverlayState
{
    Stopped,
    Running,
    Degraded,
    Failed
};

enum class RuntimeOverlayBackend
{
    None,
    LayerShell,
    QWindowFallback
};

enum class RuntimeCompositor
{
    Unknown,
    Mango,
    Niri,
    Libinput
};

struct ShakeRuntimeStatus
{
    bool runtime_active = false;
    RuntimeInputState input_state = RuntimeInputState::Stopped;
    RuntimeOverlayState overlay_state = RuntimeOverlayState::Stopped;
    RuntimeOverlayBackend overlay_backend = RuntimeOverlayBackend::None;
    RuntimeCompositor compositor = RuntimeCompositor::Unknown;
    std::string last_error;
    std::string updated_at;
};

std::string to_string(RuntimeInputState state);
std::string to_string(RuntimeOverlayState state);
std::string to_string(RuntimeOverlayBackend backend);
std::string to_string(RuntimeCompositor compositor);

RuntimeInputState runtime_input_state_from_string(const std::string& value);
RuntimeOverlayState runtime_overlay_state_from_string(const std::string& value);
RuntimeOverlayBackend runtime_overlay_backend_from_string(const std::string& value);
RuntimeCompositor runtime_compositor_from_string(const std::string& value);

class RuntimeStatusPublisher
{
public:
    RuntimeStatusPublisher();
    explicit RuntimeStatusPublisher(std::string path);

    const std::string& path() const;
    void set_path(std::string path);

    bool publish(const ShakeRuntimeStatus& status) const;
    static std::string default_path();
    static ShakeRuntimeStatus read();
    static bool exists();

private:
    std::string m_path;
};

ShakeRuntimeStatus stopped_runtime_status();

} // namespace waymouse
