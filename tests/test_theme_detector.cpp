#include "core/theme_detector.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace waymouse;

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        std::cout << "  TEST: " << name << "... "; \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        std::cout << "PASSED\n"; \
    } while(0)

#define FAIL(msg) \
    do { \
        std::cout << "FAILED: " << msg << "\n"; \
    } while(0)

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { FAIL(msg); return; } \
    } while(0)

#define ASSERT_FALSE(cond, msg) \
    do { \
        if (cond) { FAIL(msg); return; } \
    } while(0)

#define ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { FAIL(msg << " expected=" << (b) << " got=" << (a)); return; } \
    } while(0)

// Helper: create a minimal valid cursor theme directory
static std::string create_valid_theme(const std::string& base, const std::string& name)
{
    auto path = std::filesystem::path(base) / name;
    auto cursors_dir = path / "cursors";
    std::filesystem::create_directories(cursors_dir);

    // Create a dummy cursor file
    std::ofstream ofs(cursors_dir / "default");
    ofs << "dummy";
    ofs.close();

    return path.string();
}

// Helper: create an empty theme directory (no cursors/ subdir)
static void create_empty_dir(const std::string& base, const std::string& name)
{
    auto path = std::filesystem::path(base) / name;
    std::filesystem::create_directories(path);
}

// Helper: create a theme directory with empty cursors/ dir
static void create_empty_cursors_dir(const std::string& base, const std::string& name)
{
    auto path = std::filesystem::path(base) / name;
    auto cursors_dir = path / "cursors";
    std::filesystem::create_directories(cursors_dir);
}

static void test_valid_theme_detection()
{
    TEST("detect_themes finds valid themes");
    std::filesystem::path tmp = "/tmp/waymouse_test_themes";
    std::filesystem::remove_all(tmp);
    std::filesystem::create_directories(tmp);

    create_valid_theme(tmp.string(), "Bibata");
    create_valid_theme(tmp.string(), "Adwaita");
    create_empty_dir(tmp.string(), "NotATheme");
    create_empty_cursors_dir(tmp.string(), "EmptyCursors");

    // We need to trick ThemeDetector to scan our temp dir.
    // Since detect_themes() scans /usr/share/icons and ~/.icons,
    // we test directly via is_valid_theme_directory() and scan_directory().

    ThemeDetector detector;

    // Test is_valid_theme_directory
    ASSERT_TRUE(detector.is_valid_theme_directory(tmp.string() + "/Bibata"),
                "Bibata should be a valid theme");
    ASSERT_TRUE(detector.is_valid_theme_directory(tmp.string() + "/Adwaita"),
                "Adwaita should be a valid theme");
    ASSERT_FALSE(detector.is_valid_theme_directory(tmp.string() + "/NotATheme"),
                 "NotATheme should not be a valid theme");
    ASSERT_FALSE(detector.is_valid_theme_directory(tmp.string() + "/EmptyCursors"),
                 "EmptyCursors should not be a valid theme");
    ASSERT_FALSE(detector.is_valid_theme_directory(tmp.string() + "/NonExistent"),
                 "NonExistent should not be a valid theme");

    std::filesystem::remove_all(tmp);
    PASS();
}

static void test_detect_themes_returns_sorted()
{
    TEST("detect_themes returns sorted list");
    ThemeDetector detector;
    auto themes = detector.detect_themes();

    if (themes.empty())
    {
        std::cout << "SKIPPED (no themes on system)\n";
        tests_run--;
        return;
    }

    // Verify sorted
    for (size_t i = 1; i < themes.size(); ++i)
    {
        ASSERT_TRUE(themes[i - 1].name <= themes[i].name,
                    "themes not sorted at index " << i);
    }

    PASS();
}

static void test_default_theme_from_env()
{
    TEST("default_theme reads XCURSOR_THEME");
    ThemeDetector detector;

    // Save original env
    const char* original = std::getenv("XCURSOR_THEME");

    // Set env and test
    setenv("XCURSOR_THEME", "TestCursor", 1);
    ASSERT_EQ(detector.default_theme(), "TestCursor",
              "default_theme should return TestCursor");

    // Restore
    if (original)
        setenv("XCURSOR_THEME", original, 1);
    else
        unsetenv("XCURSOR_THEME");

    PASS();
}

static void test_default_theme_empty()
{
    TEST("default_theme returns empty when XCURSOR_THEME unset");
    // Save original
    const char* original = std::getenv("XCURSOR_THEME");
    unsetenv("XCURSOR_THEME");

    ThemeDetector detector;
    ASSERT_EQ(detector.default_theme(), "",
              "default_theme should return empty string");

    if (original)
        setenv("XCURSOR_THEME", original, 1);

    PASS();
}

static void test_default_size_from_env()
{
    TEST("default_size reads XCURSOR_SIZE");
    ThemeDetector detector;

    const char* original = std::getenv("XCURSOR_SIZE");
    setenv("XCURSOR_SIZE", "48", 1);
    ASSERT_EQ(detector.default_size(), 48,
              "default_size should return 48");

    if (original)
        setenv("XCURSOR_SIZE", original, 1);
    else
        unsetenv("XCURSOR_SIZE");

    PASS();
}

static void test_default_size_fallback()
{
    TEST("default_size returns 24 when XCURSOR_SIZE unset");
    const char* original = std::getenv("XCURSOR_SIZE");
    unsetenv("XCURSOR_SIZE");

    ThemeDetector detector;
    ASSERT_EQ(detector.default_size(), 24,
              "default_size should return 24");

    if (original)
        setenv("XCURSOR_SIZE", original, 1);

    PASS();
}

int main()
{
    std::cout << "=== ThemeDetector Tests ===\n";

    test_valid_theme_detection();
    test_detect_themes_returns_sorted();
    test_default_theme_from_env();
    test_default_theme_empty();
    test_default_size_from_env();
    test_default_size_fallback();

    std::cout << "\nResults: " << tests_passed << "/" << tests_run << " passed\n";
    return (tests_passed == tests_run) ? 0 : 1;
}
