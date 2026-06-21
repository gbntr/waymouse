#include "core/theme_detector.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace waymouse {

std::vector<CursorTheme> ThemeDetector::scan_directory(const std::string& base_path,
                                                       bool is_user) const
{
    std::vector<CursorTheme> themes;
    std::error_code ec;

    if (!std::filesystem::exists(base_path, ec) || ec)
        return themes;

    for (const auto& entry : std::filesystem::directory_iterator(base_path, ec))
    {
        if (ec)
            continue;

        if (!entry.is_directory(ec) || ec)
            continue;

        std::string theme_path = entry.path().string();
        if (is_valid_theme_directory(theme_path))
        {
            CursorTheme theme;
            theme.name = entry.path().filename().string();
            theme.path = theme_path;
            theme.is_user_theme = is_user;
            themes.push_back(std::move(theme));
        }
    }

    return themes;
}

bool ThemeDetector::is_valid_theme_directory(const std::string& path) const
{
    std::error_code ec;
    auto cursors_path = std::filesystem::path(path) / "cursors";

    if (!std::filesystem::exists(cursors_path, ec) || ec)
        return false;

    if (!std::filesystem::is_directory(cursors_path, ec) || ec)
        return false;

    // Must contain at least one file inside the cursors/ directory
    for (const auto& entry : std::filesystem::directory_iterator(cursors_path, ec))
    {
        if (ec)
            continue;

        if (entry.is_regular_file(ec) || entry.is_symlink(ec))
            return true;
    }

    return false;
}

std::vector<CursorTheme> ThemeDetector::detect_themes() const
{
    std::vector<CursorTheme> themes;

    // Scan system themes first
    auto system_themes = scan_directory("/usr/share/icons", false);
    themes.insert(themes.end(), system_themes.begin(), system_themes.end());

    // Scan user themes — user themes shadow system themes
    const char* home = std::getenv("HOME");
    std::string user_icons =
        (home ? std::string(home) : std::string("/tmp")) + "/.icons";
    auto user_themes = scan_directory(user_icons, true);

    for (auto& user_theme : user_themes)
    {
        auto it = std::find_if(themes.begin(), themes.end(),
                               [&user_theme](const CursorTheme& t) {
                                   return t.name == user_theme.name;
                               });

        if (it != themes.end())
        {
            // Replace system theme with user theme
            *it = std::move(user_theme);
        }
        else
        {
            themes.push_back(std::move(user_theme));
        }
    }

    // Sort alphabetically by name
    std::sort(themes.begin(), themes.end(),
              [](const CursorTheme& a, const CursorTheme& b) {
                  return a.name < b.name;
              });

    return themes;
}

std::string ThemeDetector::default_theme() const
{
    const char* theme = std::getenv("XCURSOR_THEME");
    return theme ? std::string(theme) : std::string();
}

int ThemeDetector::default_size() const
{
    const char* size = std::getenv("XCURSOR_SIZE");
    if (!size)
        return 24;

    try
    {
        return std::stoi(size);
    }
    catch (const std::exception&)
    {
        return 24;
    }
}

} // namespace waymouse
