#include "core/pointer_manager.hpp"
#include "core/theme_detector.hpp"
#include "backends/cursor_backend.hpp"
#include <QTest>
#include <QSignalSpy>
#include <string>
#include <vector>

using namespace waymouse;

// Mock CursorBackend — records calls and returns configurable success
class MockCursorBackend : public CursorBackend
{
public:
    std::string last_theme;
    int last_size = 0;
    int apply_count = 0;
    bool runtime_support = false;

    bool apply_cursor_theme(const std::string& theme, int size) override
    {
        last_theme = theme;
        last_size = size;
        apply_count++;
        return runtime_support;
    }

    bool supports_runtime_cursor_change() const override
    {
        return runtime_support;
    }

    std::string name() const override
    {
        return "mock-cursor";
    }
};

// Mock ThemeDetector — returns a fixed list
class MockThemeDetector : public ThemeDetector
{
public:
    std::vector<CursorTheme> detect_themes() const
    {
        CursorTheme t1{"Adwaita", "/usr/share/icons/Adwaita", false};
        CursorTheme t2{"Bibata", "/home/user/.icons/Bibata", true};
        CursorTheme t3{"DMZ-White", "/usr/share/icons/DMZ-White", false};
        return {t1, t2, t3};
    }
};

class TestPointerManager : public QObject
{
    Q_OBJECT

private slots:
    void test_set_theme()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);

        QSignalSpy changed_spy(&mgr, &PointerManager::changed);
        QSignalSpy runtime_spy(&mgr, &PointerManager::runtime_applied);

        mgr.set_theme("Bibata");

        QCOMPARE(mgr.theme(), std::string("Bibata"));
        QCOMPARE(changed_spy.count(), 1);
        QCOMPARE(runtime_spy.count(), 1);
        QCOMPARE(runtime_spy.at(0).at(0).toBool(), false); // mock returns false
        QCOMPARE(backend.last_theme, std::string("Bibata"));
        QCOMPARE(backend.apply_count, 1);
    }

    void test_set_size()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);

        QSignalSpy changed_spy(&mgr, &PointerManager::changed);

        mgr.set_size(32);

        QCOMPARE(mgr.size(), 32);
        QCOMPARE(backend.last_size, 32);
        QCOMPARE(changed_spy.count(), 1);
    }

    void test_set_size_normalizes()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);

        // Odd values should round up to even
        mgr.set_size(17);
        QCOMPARE(mgr.size(), 18);

        // Out of range should clamp
        mgr.set_size(100);
        QCOMPARE(mgr.size(), 64);

        // Negative should clamp to 16
        mgr.set_size(-5);
        QCOMPARE(mgr.size(), 16);

        // Value 65 should clamp to 64
        mgr.set_size(65);
        QCOMPARE(mgr.size(), 64);

        // Value 15 should clamp to 16
        mgr.set_size(15);
        QCOMPARE(mgr.size(), 16);
    }

    void test_reset_to_defaults()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);

        // Set some values first
        mgr.set_theme("Bibata");
        mgr.set_size(48);

        QCOMPARE(mgr.theme(), std::string("Bibata"));
        QCOMPARE(mgr.size(), 48);

        // Reset
        mgr.reset_to_defaults();

        QCOMPARE(mgr.theme(), std::string(""));
        QCOMPARE(mgr.size(), 24);
    }

    void test_available_themes()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);

        auto themes = mgr.available_themes();
        QCOMPARE(themes.size(), size_t(3));
        QCOMPARE(themes[0].name, std::string("Adwaita"));
        QCOMPARE(themes[1].name, std::string("Bibata"));
        QCOMPARE(themes[2].name, std::string("DMZ-White"));
    }

    void test_can_apply_runtime()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        backend.runtime_support = false;
        PointerManager mgr(&detector, &backend);

        QCOMPARE(mgr.can_apply_runtime(), false);

        backend.runtime_support = true;
        QCOMPARE(mgr.can_apply_runtime(), true);
    }

    void test_runtime_applied_signal()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        backend.runtime_support = true;
        PointerManager mgr(&detector, &backend);

        QSignalSpy runtime_spy(&mgr, &PointerManager::runtime_applied);

        mgr.set_theme("Adwaita");
        QCOMPARE(runtime_spy.count(), 1);
        // Mock returns true for runtime support
        QCOMPARE(runtime_spy.at(0).at(0).toBool(), true);
    }

    void test_load_config()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);

        QSignalSpy changed_spy(&mgr, &PointerManager::changed);

        PointerConfig cfg;
        cfg.theme = "DMZ-White";
        cfg.size = 36;
        mgr.load_config(cfg);

        QCOMPARE(mgr.theme(), std::string("DMZ-White"));
        QCOMPARE(mgr.size(), 36);
        QCOMPARE(changed_spy.count(), 1);

        // load_config should NOT call apply (only set/save triggers apply)
        QCOMPARE(backend.apply_count, 0);
    }

    void test_load_config_normalizes_size()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);

        PointerConfig cfg;
        cfg.size = 17; // odd
        mgr.load_config(cfg);

        QCOMPARE(mgr.size(), 18);

        cfg.size = 200;
        mgr.load_config(cfg);
        QCOMPARE(mgr.size(), 64);
    }
};

QTEST_MAIN(TestPointerManager)
#include "test_pointer_manager.moc"
