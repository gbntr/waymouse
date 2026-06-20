#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

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

inline double normalize_half_step(double value, double min, double max, double fallback)
{
    if (!std::isfinite(value) || value < min || value > max)
        return fallback;

    value = std::round(value * 2.0) / 2.0;
    return std::clamp(value, min, max);
}

inline std::string normalize_sensitivity(std::string value)
{
    if (value == "low" || value == "medium" || value == "high")
        return value;
    return "medium";
}

inline ShakeConfig normalize_shake_config(ShakeConfig cfg)
{
    cfg.sensitivity = normalize_sensitivity(std::move(cfg.sensitivity));
    cfg.duration = normalize_half_step(cfg.duration, 0.5, 5.0, 1.5);
    cfg.scale = normalize_half_step(cfg.scale, 2.0, 5.0, 3.0);
    return cfg;
}

} // namespace waymouse
