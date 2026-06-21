#include "core/mango_ipc_client.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace waymouse;

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { ++tests_run; std::cout << "  TEST: " << name << "... "; } while (0)
#define PASS() do { ++tests_passed; std::cout << "PASSED\n"; } while (0)
#define FAIL(msg) do { std::cout << "FAILED: " << msg << "\n"; } while (0)
#define ASSERT_TRUE(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while (0)

static std::string make_stub_mmsg(const std::string& dir)
{
    std::filesystem::create_directories(dir);
    const std::string path = dir + "/mmsg";
    std::ofstream out(path);
    out << "#!/bin/sh\n";
    out << "printf '%s' '[{\"name\":\"HDMI-A-1\",\"x\":0,\"y\":0,\"width\":1920,\"height\":1080,\"scale\":1.0,\"focused\":true}]'\n";
    out.close();
    std::filesystem::permissions(path,
        std::filesystem::perms::owner_exec | std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::add);
    return path;
}

static void test_parse_stub_output()
{
    TEST("mmsg JSON parsing");

    const std::string dir = "/tmp/waymouse_mmsg_stub";
    std::filesystem::remove_all(dir);
    make_stub_mmsg(dir);
    const char* current_path = std::getenv("PATH");
    setenv("PATH", (dir + ":" + (current_path ? std::string(current_path) : std::string{})).c_str(), 1);
    setenv("MANGO_INSTANCE_SIGNATURE", "test-signature", 1);

    MangoIpcClient client;
    std::string error;
    auto layouts = client.query_monitor_layouts(&error);
    ASSERT_TRUE(error.empty(), error.c_str());
    ASSERT_TRUE(layouts.size() == 1, "should parse one monitor");
    ASSERT_TRUE(layouts.front().focused, "monitor should be focused");
    PASS();
}

static void test_missing_env_is_degraded()
{
    TEST("missing mango env degrades gracefully");

    unsetenv("MANGO_INSTANCE_SIGNATURE");
    MangoIpcClient client;
    std::string error;
    auto layouts = client.query_monitor_layouts(&error);
    ASSERT_TRUE(layouts.empty(), "layouts should be empty");
    ASSERT_TRUE(!error.empty(), "should report an error");
    PASS();
}

static void test_empty_env_is_not_available()
{
    TEST("empty mango env is unavailable");

    setenv("MANGO_INSTANCE_SIGNATURE", "", 1);
    MangoIpcClient client;
    ASSERT_TRUE(!client.is_available(), "empty signature should not be available");
    PASS();
}

int main()
{
    test_parse_stub_output();
    test_missing_env_is_degraded();
    test_empty_env_is_not_available();

    std::cout << "\nResults: " << tests_passed << "/" << tests_run << " passed\n";
    return tests_passed == tests_run ? 0 : 1;
}
