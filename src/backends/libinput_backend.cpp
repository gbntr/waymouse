#include "backends/libinput_backend.hpp"
#include <iostream>

namespace waymouse {

bool LibinputBackend::apply(const Device& device, const Config& cfg)
{
    std::cerr << "[libinput] apply not yet implemented for " << device.name << "\n";
    (void)cfg;
    return false;
}

bool LibinputBackend::supports(const Device& /*device*/) const
{
    return true; // fallback is always available
}

std::string LibinputBackend::name() const
{
    return "libinput";
}

} // namespace waymouse
