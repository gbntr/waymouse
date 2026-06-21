#include "core/config_manager.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace waymouse {

namespace {

double to_double(const toml::value& v, double fallback)
{
    if (v.is_floating())
        return v.as_floating();
    if (v.is_integer())
        return static_cast<double>(v.as_integer());
    return fallback;
}

int to_int(const toml::value& v, int fallback)
{
    if (v.is_integer())
        return static_cast<int>(v.as_integer());
    if (v.is_floating())
        return static_cast<int>(std::lround(v.as_floating()));
    return fallback;
}

} // namespace

ConfigManager::ConfigManager()
{
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    std::string base = xdg ? std::string(xdg) : (std::string(std::getenv("HOME") ? std::getenv("HOME") : ".") + "/.config");
    m_path = base + "/waymouse/config.toml";
}

std::string ConfigManager::config_path() const
{
    return m_path;
}

bool ConfigManager::load()
{
    if (!std::filesystem::exists(m_path))
    {
        // Initialize as empty table so .contains() doesn't throw type_error
        m_data = toml::value(toml::table{});
        return true;
    }

    try
    {
        m_data = toml::parse(m_path);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to load config: " << e.what() << "\n";
        // Ensure m_data is a valid table after parse failure
        m_data = toml::value(toml::table{});
        return false;
    }
    return true;
}

bool ConfigManager::save()
{
    try
    {
        std::filesystem::create_directories(std::filesystem::path(m_path).parent_path());
        std::ofstream ofs(m_path);
        ofs << m_data;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to save config: " << e.what() << "\n";
        return false;
    }
    return true;
}

std::optional<Config> ConfigManager::get(const std::string& device_name) const
{
    if (!m_data.contains("device"))
        return std::nullopt;

    auto devices = toml::find(m_data, "device");
    if (!devices.is_table())
        return std::nullopt;

    const auto& tbl = devices.as_table();
    auto it = tbl.find(device_name);
    if (it == tbl.end())
        return std::nullopt;

    const auto& v = it->second;
    Config cfg;
    if (v.contains("accel_speed"))
    {
        auto accel_speed = toml::find(v, "accel_speed");
        cfg.accel_speed = to_double(accel_speed, cfg.accel_speed);
    }
    if (v.contains("accel_profile"))
        cfg.accel_profile = toml::find<std::string>(v, "accel_profile");
    if (v.contains("natural_scroll"))
        cfg.natural_scroll = toml::find<bool>(v, "natural_scroll");
    if (v.contains("left_handed"))
        cfg.left_handed = toml::find<bool>(v, "left_handed");

    return cfg;
}

void ConfigManager::set(const std::string& device_name, const Config& cfg)
{
    if (!m_data.contains("device"))
        m_data["device"] = toml::value{};

    toml::value dev;
    dev["accel_speed"] = cfg.accel_speed;
    dev["accel_profile"] = cfg.accel_profile;
    dev["natural_scroll"] = cfg.natural_scroll;
    dev["left_handed"] = cfg.left_handed;

    m_data["device"][device_name] = std::move(dev);
}

std::optional<PointerConfig> ConfigManager::get_pointer() const
{
    if (!m_data.contains("pointer"))
        return std::nullopt;

    auto ptr = toml::find(m_data, "pointer");
    if (!ptr.is_table())
        return std::nullopt;

    PointerConfig cfg;
    if (ptr.contains("theme"))
        cfg.theme = toml::find<std::string>(ptr, "theme");
    if (ptr.contains("size"))
    {
        auto size = toml::find(ptr, "size");
        cfg.size = to_int(size, cfg.size);
    }

    return cfg;
}

void ConfigManager::set_pointer(const PointerConfig& cfg)
{
    toml::value ptr;
    ptr["theme"] = cfg.theme;
    ptr["size"] = cfg.size;

    m_data["pointer"] = std::move(ptr);
}

std::optional<ShakeConfig> ConfigManager::get_shake() const
{
    if (!m_data.contains("shake"))
        return std::nullopt;

    auto shake = toml::find(m_data, "shake");
    if (!shake.is_table())
        return std::nullopt;

    ShakeConfig cfg;
    if (shake.contains("enabled"))
        cfg.enabled = toml::find<bool>(shake, "enabled");
    if (shake.contains("sensitivity"))
        cfg.sensitivity = toml::find<std::string>(shake, "sensitivity");
    if (shake.contains("duration"))
    {
        auto duration = toml::find(shake, "duration");
        cfg.duration = to_double(duration, cfg.duration);
    }
    if (shake.contains("scale"))
    {
        auto scale = toml::find(shake, "scale");
        cfg.scale = to_double(scale, cfg.scale);
    }

    return normalize_shake_config(std::move(cfg));
}

void ConfigManager::set_shake(const ShakeConfig& cfg)
{
    ShakeConfig normalized = normalize_shake_config(cfg);

    toml::value shake;
    shake["enabled"] = normalized.enabled;
    shake["sensitivity"] = normalized.sensitivity;
    shake["duration"] = normalized.duration;
    shake["scale"] = normalized.scale;

    m_data["shake"] = std::move(shake);
}

} // namespace waymouse
