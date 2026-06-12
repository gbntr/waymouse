#include "backends/mango_backend.hpp"
#include <iostream>

namespace waymouse {

bool MangoBackend::apply(const Device& device, const Config& cfg)
{
    std::cerr << "[mango] apply not yet implemented for " << device.name << "\n";
    (void)cfg;
    return false;
}

bool MangoBackend::supports(const Device& /*device*/) const
{
    return true; // TODO: check if compositor is Mango and protocol available
}

std::string MangoBackend::name() const
{
    return "mango";
}

} // namespace waymouse
