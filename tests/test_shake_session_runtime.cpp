#include "core/shake_session_runtime.hpp"
#include "core/config_manager.hpp"
#include "core/pointer_manager.hpp"
#include "core/theme_detector.hpp"
#include "backends/cursor_backend.hpp"

#include <QApplication>
#include <QTest>

#include <cstdlib>
#include <filesystem>

using namespace waymouse;

class MockCursorBackend : public CursorBackend
{
public:
    bool apply_cursor_theme(const std::string&, int) override { return true; }
    bool supports_runtime_cursor_change() const override { return true; }
    std::string name() const override { return "mock"; }
};

class MockThemeDetector : public ThemeDetector
{
public:
    std::vector<CursorTheme> detect_themes() const override
    {
        return {CursorTheme{"Adwaita", "/usr/share/icons/Adwaita", true}};
    }
};

class TestShakeSessionRuntime : public QObject
{
    Q_OBJECT

private slots:
    void test_start_publishes_status_and_lock()
    {
        const std::string runtime_dir = "/tmp/waymouse_session_runtime_test";
        const std::string config_dir = runtime_dir + "/config";
        std::filesystem::remove_all(runtime_dir);
        std::filesystem::create_directories(runtime_dir);
        std::filesystem::create_directories(config_dir + "/waymouse");
        setenv("XDG_RUNTIME_DIR", runtime_dir.c_str(), 1);
        setenv("XDG_CONFIG_HOME", config_dir.c_str(), 1);

        ConfigManager cfg_mgr;
        cfg_mgr.load();
        cfg_mgr.set_shake(ShakeConfig{});
        cfg_mgr.save();

        MockThemeDetector theme_detector;
        MockCursorBackend cursor_backend;
        PointerManager pointer_mgr(&theme_detector, &cursor_backend);

        ShakeSessionRuntime runtime(&cfg_mgr, &pointer_mgr, "mango");
        QVERIFY(runtime.start());
        QVERIFY(runtime.is_running());
        QVERIFY(std::filesystem::exists(runtime_dir + "/waymouse/shake-status.json"));
        runtime.stop();

        std::filesystem::remove_all(runtime_dir);
    }
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);

    TestShakeSessionRuntime tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_shake_session_runtime.moc"
