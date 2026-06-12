#include "core/config_manager.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace waymouse {

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

    const auto& devices = toml::find(m_data, "device");
    if (!devices.is_table())
        return std::nullopt;

    const auto& tbl = devices.as_table();
    auto it = tbl.find(device_name);
    if (it == tbl.end())
        return std::nullopt;

    const auto& v = it->second;
    Config cfg;
    if (v.contains("accel_speed"))
        cfg.accel_speed = toml::find<double>(v, "accel_speed");
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

    const auto& ptr = toml::find(m_data, "pointer");
    if (!ptr.is_table())
        return std::nullopt;

    PointerConfig cfg;
    if (ptr.contains("theme"))
        cfg.theme = toml::find<std::string>(ptr, "theme");
    if (ptr.contains("size"))
        cfg.size = toml::find<int>(ptr, "size");

    return cfg;
}

void ConfigManager::set_pointer(const PointerConfig& cfg)
{
    toml::value ptr;
    ptr["theme"] = cfg.theme;
    ptr["size"] = cfg.size;

    m_data["pointer"] = std::move(ptr);
}

} // namespace waymouse
