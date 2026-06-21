#include "backends/env_cursor_backend.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace waymouse {

bool EnvCursorBackend::apply_cursor_theme(const std::string& theme, int size)
{
    // Set environment variables in the current process (best-effort for this session)
    if (theme.empty())
    {
        unsetenv("XCURSOR_THEME");
    }
    else
    {
        setenv("XCURSOR_THEME", theme.c_str(), 1);
    }
    setenv("XCURSOR_SIZE", std::to_string(size).c_str(), 1);

    // Persist to environment.d for next session
    write_environment_d_file(theme, size);

    // Always return false: we did not apply to the running compositor
    return false;
}

bool EnvCursorBackend::supports_runtime_cursor_change() const
{
    return false;
}

std::string EnvCursorBackend::name() const
{
    return "env-cursor";
}

void EnvCursorBackend::write_environment_d_file(const std::string& theme,
                                                int size) const
{
    std::error_code ec;

    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    std::string config_base =
        xdg_config ? std::string(xdg_config)
                   : (std::string(std::getenv("HOME") ? std::getenv("HOME") : ".") +
                      "/.config");

    auto env_d_dir = std::filesystem::path(config_base) / "environment.d";
    std::filesystem::create_directories(env_d_dir, ec);

    auto file_path = env_d_dir / "waymouse-cursor.conf";
    std::ofstream ofs(file_path);
    if (!ofs)
        return;

    if (!theme.empty())
    {
        ofs << "XCURSOR_THEME=" << theme << "\n";
    }
    ofs << "XCURSOR_SIZE=" << size << "\n";
}

} // namespace waymouse
