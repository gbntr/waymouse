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
    virtual ~ThemeDetector() = default;
    virtual std::vector<CursorTheme> detect_themes() const;
    virtual std::string default_theme() const;
    virtual int default_size() const;
    bool is_valid_theme_directory(const std::string& path) const;

private:
    std::vector<CursorTheme> scan_directory(const std::string& base_path,
                                            bool is_user) const;
};

} // namespace waymouse
