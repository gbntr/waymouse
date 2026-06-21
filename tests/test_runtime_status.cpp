#include "core/runtime_status.hpp"
#include "core/runtime_lock.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

using namespace waymouse;

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { ++tests_run; std::cout << "  TEST: " << name << "... "; } while (0)
#define PASS() do { ++tests_passed; std::cout << "PASSED\n"; } while (0)
#define FAIL(msg) do { std::cout << "FAILED: " << msg << "\n"; } while (0)
#define ASSERT_TRUE(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while (0)

static void test_status_round_trip()
{
    TEST("status publish/read round trip");

    const std::string base = "/tmp/waymouse_runtime_status_test";
    std::filesystem::remove_all(base);
    std::filesystem::create_directories(base);
    setenv("XDG_RUNTIME_DIR", base.c_str(), 1);

    const std::string path = base + "/waymouse/shake-status.json";

    RuntimeStatusPublisher publisher(path);
    ShakeRuntimeStatus status;
    status.runtime_active = true;
    status.input_state = RuntimeInputState::Running;
    status.overlay_state = RuntimeOverlayState::Running;
    status.overlay_backend = RuntimeOverlayBackend::LayerShell;
    status.compositor = RuntimeCompositor::Mango;
    status.last_error = "";
    ASSERT_TRUE(publisher.publish(status), "publish should succeed");

    auto loaded = RuntimeStatusPublisher::read();
    ASSERT_TRUE(loaded.runtime_active == true, "runtime_active should round-trip");
    ASSERT_TRUE(loaded.input_state == RuntimeInputState::Running, "input_state should round-trip");
    ASSERT_TRUE(loaded.overlay_backend == RuntimeOverlayBackend::LayerShell, "overlay backend should round-trip");
    PASS();
}

static void test_string_mappings()
{
    TEST("enum string mappings");

    ASSERT_TRUE(to_string(RuntimeInputState::PermissionDenied) == "permission_denied", "input state string");
    ASSERT_TRUE(to_string(RuntimeOverlayBackend::QWindowFallback) == "qwindow-fallback", "overlay backend string");
    ASSERT_TRUE(runtime_compositor_from_string("mango") == RuntimeCompositor::Mango, "compositor mapping");
    PASS();
}

static void test_runtime_lock_single_instance()
{
    TEST("runtime lock enforces single instance");

    const std::string lock_path = "/tmp/waymouse_runtime_status_test/shake-runtime.lock";
    RuntimeLock first(lock_path);
    RuntimeLock second(lock_path);
    ASSERT_TRUE(first.acquire(), "first lock should acquire");
    ASSERT_TRUE(!second.acquire(), "second lock should fail while first is held");
    first.release();
    PASS();
}

int main()
{
    test_status_round_trip();
    test_string_mappings();
    test_runtime_lock_single_instance();

    std::cout << "\nResults: " << tests_passed << "/" << tests_run << " passed\n";
    return tests_passed == tests_run ? 0 : 1;
}
