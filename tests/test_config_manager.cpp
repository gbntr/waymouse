#include "core/config_manager.hpp"
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

// Helper: create a temp config file
static void write_config(const std::string& path, const std::string& content)
{
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    ofs << content;
    ofs.close();
}

// Helper: set up an isolated XDG_CONFIG_HOME dir for the test and expose
// the path the ConfigManager will actually use. ConfigManager always
// reads/writes $XDG_CONFIG_HOME/waymouse/config.toml, so callers must
// write to the same path (or rely on save() to create it).
struct ScopedXdgConfig {
    std::string dir;
    std::string config_path;
    ScopedXdgConfig(const std::string& sub)
    {
        dir = "/tmp/waymouse_test_" + sub;
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
        setenv("XDG_CONFIG_HOME", dir.c_str(), 1);
        config_path = dir + "/waymouse/config.toml";
        std::filesystem::create_directories(dir + "/waymouse");
    }
    ~ScopedXdgConfig() { std::filesystem::remove_all(dir); }
};

static void test_get_pointer_returns_nullopt_when_missing()
{
    TEST("get_pointer returns nullopt when [pointer] missing");

    ScopedXdgConfig env("1");

    write_config(env.config_path, R"(
[device."My Mouse"]
accel_speed = 0.5
)");

    ConfigManager mgr;
    bool loaded = mgr.load();
    ASSERT_TRUE(loaded, "failed to load config");

    auto ptr = mgr.get_pointer();
    ASSERT_FALSE(ptr.has_value(), "should return nullopt");

    PASS();
}

static void test_get_pointer_returns_config()
{
    TEST("get_pointer returns saved config");

    ScopedXdgConfig env("2");

    write_config(env.config_path, R"(
[pointer]
theme = "Bibata"
size = 32

[device."My Mouse"]
accel_speed = 0.5
)");

    ConfigManager mgr;
    bool loaded = mgr.load();
    ASSERT_TRUE(loaded, "failed to load config");

    auto ptr = mgr.get_pointer();
    ASSERT_TRUE(ptr.has_value(), "should have pointer config");
    ASSERT_TRUE(ptr->theme == "Bibata", "theme should be Bibata");
    ASSERT_TRUE(ptr->size == 32, "size should be 32");

    PASS();
}

static void test_set_pointer_saves_correctly()
{
    TEST("set_pointer saves correctly");

    ScopedXdgConfig env("3");

    ConfigManager mgr;
    mgr.load();

    PointerConfig cfg;
    cfg.theme = "DMZ-White";
    cfg.size = 48;
    mgr.set_pointer(cfg);
    mgr.save();

    // Reload and verify
    ConfigManager mgr2;
    mgr2.load();
    auto ptr2 = mgr2.get_pointer();
    ASSERT_TRUE(ptr2.has_value(), "should have pointer config after reload");
    ASSERT_TRUE(ptr2->theme == "DMZ-White", "theme should be DMZ-White");
    ASSERT_TRUE(ptr2->size == 48, "size should be 48");

    PASS();
}

static void test_set_pointer_preserves_device_config()
{
    TEST("set_pointer does not overwrite device config");

    ScopedXdgConfig env("4");

    // Set device config first
    ConfigManager mgr;
    mgr.load();

    Config dev_cfg;
    dev_cfg.accel_speed = 0.5;
    dev_cfg.accel_profile = "flat";
    dev_cfg.natural_scroll = true;
    dev_cfg.left_handed = false;
    mgr.set("Logitech G102", dev_cfg);
    mgr.save();

    // Now set pointer config
    PointerConfig ptr_cfg;
    ptr_cfg.theme = "Bibata";
    ptr_cfg.size = 32;
    mgr.set_pointer(ptr_cfg);
    mgr.save();

    // Reload and verify both exist
    ConfigManager mgr2;
    mgr2.load();

    auto dev = mgr2.get("Logitech G102");
    ASSERT_TRUE(dev.has_value(), "device config should exist");
    ASSERT_TRUE(dev->accel_speed == 0.5, "accel_speed should be 0.5");
    ASSERT_TRUE(dev->accel_profile == "flat", "accel_profile should be flat");
    ASSERT_TRUE(dev->natural_scroll == true, "natural_scroll should be true");

    auto ptr = mgr2.get_pointer();
    ASSERT_TRUE(ptr.has_value(), "pointer config should exist");
    ASSERT_TRUE(ptr->theme == "Bibata", "theme should be Bibata");
    ASSERT_TRUE(ptr->size == 32, "size should be 32");

    PASS();
}

static void test_get_pointer_returns_defaults_for_missing_fields()
{
    TEST("get_pointer returns defaults for missing fields");

    ScopedXdgConfig env("5");

    write_config(env.config_path, R"(
[pointer]
theme = "Adwaita"
)");

    ConfigManager mgr;
    mgr.load();

    auto ptr = mgr.get_pointer();
    ASSERT_TRUE(ptr.has_value(), "should have pointer config");
    ASSERT_TRUE(ptr->theme == "Adwaita", "theme should be Adwaita");
    // size not in file, should be default 0 (from struct)
    // Actually the struct default is 24, but since no 'size' field,
    // it won't be found and will remain 24 from PointerConfig default
    ASSERT_TRUE(ptr->size == 24, "default size should be 24");

    PASS();
}

int main()
{
    std::cout << "=== ConfigManager Pointer Tests ===\n";

    test_get_pointer_returns_nullopt_when_missing();
    test_get_pointer_returns_config();
    test_set_pointer_saves_correctly();
    test_set_pointer_preserves_device_config();
    test_get_pointer_returns_defaults_for_missing_fields();

    std::cout << "\nResults: " << tests_passed << "/" << tests_run << " passed\n";
    return (tests_passed == tests_run) ? 0 : 1;
}
