#pragma once

#include <string>
#include <vector>

namespace waymouse {

struct CursorTheme
{
    std::string name;
    std::string path;   // absolute path to the theme directory
    bool is_user_theme; // true if located in ~/.icons
};

class ThemeDetector
{
public:
    std::vector<CursorTheme> detect_themes() const;
    bool is_valid_theme_directory(const std::string& path) const;
    std::string default_theme() const;
    int default_size() const;

private:
    std::vector<CursorTheme> scan_directory(const std::string& base_path,
                                            bool is_user) const;
};

} // namespace waymouse
