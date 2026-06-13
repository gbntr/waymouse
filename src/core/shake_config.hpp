#pragma once

#include <string>

namespace waymouse {

// Shake to Find configuration. Persisted under [shake] in config.toml.
// Invariants:
//   - sensitivity is one of "low", "medium", "high"
//   - duration is in [0.5, 5.0] and a multiple of 0.5
//   - scale is in [2.0, 5.0] and a multiple of 0.5
struct ShakeConfig
{
    bool enabled = true;
    std::string sensitivity = "medium";
    double duration = 1.5; // seconds
    double scale = 3.0;    // scale factor (e.g., 3.0 = 3× size)
};

} // namespace waymouse
