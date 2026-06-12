#pragma once

#include "backends/backend.hpp"
#include <memory>

namespace waymouse {

enum class BackendType {
    Mango,
    Niri,
    Libinput
};

class CompositorDetector {
public:
    CompositorDetector();
    BackendType detect() const;
    std::string compositor_name() const;

private:
    BackendType m_detected;
    std::string m_name;
};

BackendPtr create_backend(BackendType type);

} // namespace waymouse
