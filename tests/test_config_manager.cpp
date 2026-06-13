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

static void test_get_pointer_returns_nullopt_when_missing()
{
    TEST("get_pointer returns nullopt when [pointer] missing");

    std::string dir = "/tmp/waymouse_config_test_nullopt";
    std::string waymouse_dir = dir + "/waymouse";
    std::string config_path = waymouse_dir + "/config.toml";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(waymouse_dir);

    write_config(config_path, R"(
[device."My Mouse"]
accel_speed = 0.5
)");

    // Override config path
    setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

    ConfigManager mgr;
    bool loaded = mgr.load();
    ASSERT_TRUE(loaded, "failed to load config");

    auto ptr = mgr.get_pointer();
    ASSERT_FALSE(ptr.has_value(), "should return nullopt");

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_get_pointer_returns_config()
{
    TEST("get_pointer returns saved config");

    std::string dir = "/tmp/waymouse_config_test";
    std::string waymouse_dir = dir + "/waymouse";
    std::string config_path = waymouse_dir + "/config.toml";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(waymouse_dir);

    write_config(config_path, R"(
[pointer]
theme = "Bibata"
size = 32

[device."My Mouse"]
accel_speed = 0.5
)");

    setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

    ConfigManager mgr;
    bool loaded = mgr.load();
    ASSERT_TRUE(loaded, "failed to load config");

    auto ptr = mgr.get_pointer();
    ASSERT_TRUE(ptr.has_value(), "should have pointer config");
    ASSERT_TRUE(ptr->theme == "Bibata", "theme should be Bibata");
    ASSERT_TRUE(ptr->size == 32, "size should be 32");

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_set_pointer_saves_correctly()
{
    TEST("set_pointer saves correctly");

    std::string dir = "/tmp/waymouse_config_test_save";
    std::filesystem::remove_all(dir);

    // Use a unique config path
    setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

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

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_set_pointer_preserves_device_config()
{
    TEST("set_pointer does not overwrite device config");

    std::string dir = "/tmp/waymouse_config_test_preserve";
    std::filesystem::remove_all(dir);

    setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

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

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_get_pointer_returns_defaults_for_missing_fields()
{
    TEST("get_pointer returns defaults for missing fields");

    std::string dir = "/tmp/waymouse_config_test_defaults";
    std::string waymouse_dir = dir + "/waymouse";
    std::string config_path = waymouse_dir + "/config.toml";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(waymouse_dir);

    write_config(config_path, R"(
[pointer]
theme = "Adwaita"
)");

    setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

    ConfigManager mgr;
    mgr.load();

    auto ptr = mgr.get_pointer();
    ASSERT_TRUE(ptr.has_value(), "should have pointer config");
    ASSERT_TRUE(ptr->theme == "Adwaita", "theme should be Adwaita");
    // size not in file, should be default 0 (from struct)
    // Actually the struct default is 24, but since no 'size' field,
    // it won't be found and will remain 24 from PointerConfig default
    ASSERT_TRUE(ptr->size == 24, "default size should be 24");

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_get_shake_returns_nullopt_when_missing()
{
    TEST("get_shake returns nullopt when [shake] missing");

    std::string dir = "/tmp/waymouse_config_test_shake_nullopt";
    std::string waymouse_dir = dir + "/waymouse";
    std::string config_path = waymouse_dir + "/config.toml";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(waymouse_dir);

    write_config(config_path, R"(
[device."My Mouse"]
accel_speed = 0.5
)");

    setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

    ConfigManager mgr;
    bool loaded = mgr.load();
    ASSERT_TRUE(loaded, "failed to load config");

    auto shake = mgr.get_shake();
    ASSERT_FALSE(shake.has_value(), "should return nullopt");

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_get_shake_returns_config()
{
    TEST("get_shake returns saved config");

    std::string dir = "/tmp/waymouse_config_test_shake";
    std::string waymouse_dir = dir + "/waymouse";
    std::string config_path = waymouse_dir + "/config.toml";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(waymouse_dir);

    write_config(config_path, R"(
[shake]
enabled = true
sensitivity = "high"
duration = 2.0
scale = 4.0
)");

    setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

    ConfigManager mgr;
    bool loaded = mgr.load();
    ASSERT_TRUE(loaded, "failed to load config");

    auto shake = mgr.get_shake();
    ASSERT_TRUE(shake.has_value(), "should have shake config");
    ASSERT_TRUE(shake->enabled == true, "enabled should be true");
    ASSERT_TRUE(shake->sensitivity == "high", "sensitivity should be high");
    ASSERT_TRUE(shake->duration == 2.0, "duration should be 2.0");
    ASSERT_TRUE(shake->scale == 4.0, "scale should be 4.0");

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_shake_saves_correctly()
{
    TEST("set_shake round-trip");

    std::string dir = "/tmp/waymouse_config_test_shake_save";
    std::filesystem::remove_all(dir);

    setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

    ConfigManager mgr;
    mgr.load();

    ShakeConfig cfg;
    cfg.enabled = false;
    cfg.sensitivity = "low";
    cfg.duration = 3.0;
    cfg.scale = 2.5;
    mgr.set_shake(cfg);
    mgr.save();

    // Reload and verify
    ConfigManager mgr2;
    mgr2.load();
    auto shake2 = mgr2.get_shake();
    ASSERT_TRUE(shake2.has_value(), "should have shake config after reload");
    ASSERT_TRUE(shake2->enabled == false, "enabled should be false");
    ASSERT_TRUE(shake2->sensitivity == "low", "sensitivity should be low");
    ASSERT_TRUE(shake2->duration == 3.0, "duration should be 3.0");
    ASSERT_TRUE(shake2->scale == 2.5, "scale should be 2.5");

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_shake_pointer_device_coexist()
{
    TEST("[shake], [pointer], and [device] coexist");

    std::string dir = "/tmp/waymouse_config_test_coexist";
    std::filesystem::remove_all(dir);

    setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

    ConfigManager mgr;
    mgr.load();

    // Set all three sections
    PointerConfig ptr;
    ptr.theme = "Adwaita";
    ptr.size = 32;
    mgr.set_pointer(ptr);

    ShakeConfig shake;
    shake.enabled = true;
    shake.sensitivity = "medium";
    shake.duration = 1.5;
    shake.scale = 3.0;
    mgr.set_shake(shake);

    Config dev_cfg;
    dev_cfg.accel_speed = 0.8;
    mgr.set("Mouse X", dev_cfg);

    mgr.save();

    // Reload and verify all
    ConfigManager mgr2;
    mgr2.load();

    auto ptr2 = mgr2.get_pointer();
    ASSERT_TRUE(ptr2.has_value(), "pointer config should exist");
    ASSERT_TRUE(ptr2->theme == "Adwaita", "pointer theme should be Adwaita");

    auto shake2 = mgr2.get_shake();
    ASSERT_TRUE(shake2.has_value(), "shake config should exist");
    ASSERT_TRUE(shake2->enabled == true, "shake should be enabled");

    auto dev2 = mgr2.get("Mouse X");
    ASSERT_TRUE(dev2.has_value(), "device config should exist");
    ASSERT_TRUE(dev2->accel_speed == 0.8, "accel_speed should be 0.8");

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_get_shake_returns_defaults_for_missing_fields()
{
    TEST("get_shake returns defaults for missing fields");

    std::string dir = "/tmp/waymouse_config_test_shake_defaults";
    std::string waymouse_dir = dir + "/waymouse";
    std::string config_path = waymouse_dir + "/config.toml";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(waymouse_dir);

    write_config(config_path, R"(
[shake]
enabled = false
)");

    setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

    ConfigManager mgr;
    mgr.load();

    auto shake = mgr.get_shake();
    ASSERT_TRUE(shake.has_value(), "should have shake config");
    ASSERT_TRUE(shake->enabled == false, "enabled should be false");
    // Defaults for missing fields
    ASSERT_TRUE(shake->sensitivity == "medium", "default sensitivity should be medium");
    ASSERT_TRUE(shake->duration == 1.5, "default duration should be 1.5");
    ASSERT_TRUE(shake->scale == 3.0, "default scale should be 3.0");

    std::filesystem::remove_all(dir);
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

    std::cout << "\n=== ConfigManager Shake Tests ===\n";
    test_get_shake_returns_nullopt_when_missing();
    test_get_shake_returns_config();
    test_shake_saves_correctly();
    test_shake_pointer_device_coexist();
    test_get_shake_returns_defaults_for_missing_fields();

    std::cout << "\nResults: " << tests_passed << "/" << tests_run << " passed\n";
    return (tests_passed == tests_run) ? 0 : 1;
}
