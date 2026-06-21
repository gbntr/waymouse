#pragma once

#include <string>

namespace waymouse {

struct PointerConfig
{
    std::string theme; // cursor theme name; "" means "system default"
    int size = 24;     // cursor size; must be even, 16..64
};

} // namespace waymouse
