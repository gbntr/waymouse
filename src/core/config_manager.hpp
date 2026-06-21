#pragma once

#include "backends/backend.hpp"
#include "core/pointer_config.hpp"
#include "core/shake_config.hpp"
#include <string>
#include <optional>
#include <toml.hpp>

namespace waymouse {

class ConfigManager {
public:
    ConfigManager();

    bool load();
    bool save();

    std::optional<Config> get(const std::string& device_name) const;
    void set(const std::string& device_name, const Config& cfg);

    std::optional<PointerConfig> get_pointer() const;
    void set_pointer(const PointerConfig& cfg);

    std::optional<ShakeConfig> get_shake() const;
    void set_shake(const ShakeConfig& cfg);

    std::string config_path() const;

private:
    toml::value m_data;
    std::string m_path;
};

} // namespace waymouse
