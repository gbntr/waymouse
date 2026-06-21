#include "core/shake_session_runtime.hpp"
#include "core/config_manager.hpp"
#include "core/pointer_manager.hpp"
#include "core/theme_detector.hpp"
#include "backends/cursor_backend.hpp"

#include <QApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTest>

#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>

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
    void test_duplicate_runtime_fails_cleanly()
    {
        const std::string runtime_dir = "/tmp/waymouse_session_runtime_duplicate";
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

        ShakeSessionRuntime runtime1(&cfg_mgr, &pointer_mgr, "mango");
        QVERIFY(runtime1.start());

        {
            ShakeSessionRuntime runtime2(&cfg_mgr, &pointer_mgr, "mango");
            QVERIFY(!runtime2.start());
            QCOMPARE(runtime2.error(), std::string("another shake runtime is already running"));
        }

        runtime1.stop();
        std::filesystem::remove_all(runtime_dir);
    }

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

    void test_runtime_cleans_up_on_sigterm()
    {
        const std::string root_dir = "/tmp/waymouse_session_runtime_sigterm";
        const std::string runtime_dir = root_dir + "/runtime";
        const std::string config_dir = root_dir + "/config";
        std::filesystem::remove_all(root_dir);
        std::filesystem::create_directories(runtime_dir);
        std::filesystem::create_directories(config_dir + "/waymouse");

        QProcess proc;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("XDG_RUNTIME_DIR", QString::fromStdString(runtime_dir));
        env.insert("XDG_CONFIG_HOME", QString::fromStdString(config_dir));
        env.insert("QT_QPA_PLATFORM", "offscreen");
        proc.setProcessEnvironment(env);
        proc.start(QCoreApplication::applicationDirPath() + "/../waymouse", {"--shake-runtime"});
        QVERIFY(proc.waitForStarted());

        const QString status_path = QString::fromStdString(runtime_dir + "/waymouse/shake-status.json");
        QTRY_VERIFY(std::filesystem::exists(status_path.toStdString()));

        proc.terminate();
        QVERIFY(proc.waitForFinished(5000));
        QCOMPARE(proc.exitStatus(), QProcess::NormalExit);

        setenv("XDG_RUNTIME_DIR", runtime_dir.c_str(), 1);
        const auto status = RuntimeStatusPublisher::read();
        QVERIFY(!status.runtime_active);
        QCOMPARE(status.input_state, RuntimeInputState::Stopped);
    }

    void test_runtime_cleans_up_on_sigint()
    {
        const std::string root_dir = "/tmp/waymouse_session_runtime_sigint";
        const std::string runtime_dir = root_dir + "/runtime";
        const std::string config_dir = root_dir + "/config";
        std::filesystem::remove_all(root_dir);
        std::filesystem::create_directories(runtime_dir);
        std::filesystem::create_directories(config_dir + "/waymouse");

        QProcess proc;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("XDG_RUNTIME_DIR", QString::fromStdString(runtime_dir));
        env.insert("XDG_CONFIG_HOME", QString::fromStdString(config_dir));
        env.insert("QT_QPA_PLATFORM", "offscreen");
        proc.setProcessEnvironment(env);
        proc.start(QCoreApplication::applicationDirPath() + "/../waymouse", {"--shake-runtime"});
        QVERIFY(proc.waitForStarted());

        const QString status_path = QString::fromStdString(runtime_dir + "/waymouse/shake-status.json");
        QTRY_VERIFY(std::filesystem::exists(status_path.toStdString()));

        ::kill(static_cast<pid_t>(proc.processId()), SIGINT);
        QVERIFY(proc.waitForFinished(5000));
        QCOMPARE(proc.exitStatus(), QProcess::NormalExit);

        setenv("XDG_RUNTIME_DIR", runtime_dir.c_str(), 1);
        const auto status = RuntimeStatusPublisher::read();
        QVERIFY(!status.runtime_active);
        QCOMPARE(status.input_state, RuntimeInputState::Stopped);
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
