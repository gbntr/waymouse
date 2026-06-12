#include "core/compositor_detector.hpp"
#include "backends/mango_backend.hpp"
#include "backends/niri_backend.hpp"
#include "backends/libinput_backend.hpp"
#include <cstdlib>
#include <filesystem>

namespace waymouse {

CompositorDetector::CompositorDetector()
    : m_detected(BackendType::Libinput)
    , m_name("libinput")
{
    const char* desktop = std::getenv("XDG_CURRENT_DESKTOP");
    const char* niri = std::getenv("NIRI_SOCKET");

    if (niri != nullptr)
    {
        m_detected = BackendType::Niri;
        m_name = "niri";
    }
    else if (desktop != nullptr)
    {
        std::string d(desktop);
        // dwl and derived compositors often do not set XDG_CURRENT_DESKTOP
        // mango detection may require checking WAYLAND_SOCKET or process tree
        if (d.find("mango") != std::string::npos || d.find("dwl") != std::string::npos)
        {
            m_detected = BackendType::Mango;
            m_name = "mango";
        }
    }

    // Niri socket fallback
    if (m_detected == BackendType::Libinput)
    {
        std::filesystem::path runtime_dir = std::getenv("XDG_RUNTIME_DIR") ? std::getenv("XDG_RUNTIME_DIR") : "/tmp";
        // niri socket path may vary; this is a heuristic
    }
}

BackendType CompositorDetector::detect() const
{
    return m_detected;
}

std::string CompositorDetector::compositor_name() const
{
    return m_name;
}

BackendPtr create_backend(BackendType type)
{
    switch (type)
    {
    case BackendType::Mango:
        return std::make_unique<MangoBackend>();
    case BackendType::Niri:
        return std::make_unique<NiriBackend>();
    case BackendType::Libinput:
        return std::make_unique<LibinputBackend>();
    }
    return std::make_unique<LibinputBackend>();
}

} // namespace waymouse
