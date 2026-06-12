#include "backends/niri_backend.hpp"
#include <iostream>

namespace waymouse {

bool NiriBackend::apply(const Device& device, const Config& cfg)
{
    std::cerr << "[niri] apply not yet implemented for " << device.name << "\n";
    (void)cfg;
    return false;
}

bool NiriBackend::supports(const Device& /*device*/) const
{
    return true; // TODO: check if Niri IPC is available
}

std::string NiriBackend::name() const
{
    return "niri";
}

} // namespace waymouse
