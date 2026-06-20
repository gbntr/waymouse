#include "gui/shake_panel.hpp"
#include "core/config_manager.hpp"
#include "core/shake_manager.hpp"

#include <QApplication>
#include <QTest>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QMetaObject>
#include <QLabel>

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace waymouse;

class TestShakePanel : public QObject
{
    Q_OBJECT

private:
    static std::string temp_dir()
    {
        return "/tmp/waymouse_shake_panel_test";
    }

private slots:
    void test_startup_syncs_sliders()
    {
        const std::string dir = temp_dir();
        std::filesystem::remove_all(dir);
        setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

        ConfigManager cfg_mgr;
        cfg_mgr.load();

        ShakeManager mgr;
        ShakeConfig cfg;
        cfg.duration = 4.0;
        cfg.scale = 4.5;
        mgr.set_config(cfg);
        mgr.start();

        ShakePanel panel(&mgr, &cfg_mgr);

        auto* duration_slider = panel.findChild<QSlider*>("shake_duration_slider");
        auto* duration_spin = panel.findChild<QDoubleSpinBox*>("shake_duration_spin");
        auto* scale_slider = panel.findChild<QSlider*>("shake_scale_slider");
        auto* scale_spin = panel.findChild<QDoubleSpinBox*>("shake_scale_spin");

        QVERIFY(duration_slider != nullptr);
        QVERIFY(duration_spin != nullptr);
        QVERIFY(scale_slider != nullptr);
        QVERIFY(scale_spin != nullptr);

        QCOMPARE(duration_slider->value(), 8); // 4.0s -> 8
        QCOMPARE(duration_spin->value(), 4.0);
        QCOMPARE(scale_slider->value(), 9); // 4.5x -> 9
        QCOMPARE(scale_spin->value(), 4.5);

        mgr.stop();
        std::filesystem::remove_all(dir);
    }

    void test_duration_slider_updates_manager_and_config()
    {
        const std::string dir = temp_dir() + "_duration";
        std::filesystem::remove_all(dir);
        setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

        ConfigManager cfg_mgr;
        cfg_mgr.load();

        ShakeManager mgr;
        mgr.start();
        ShakePanel panel(&mgr, &cfg_mgr);

        auto* duration_slider = panel.findChild<QSlider*>("shake_duration_slider");
        auto* duration_spin = panel.findChild<QDoubleSpinBox*>("shake_duration_spin");

        QVERIFY(duration_slider != nullptr);
        QVERIFY(duration_spin != nullptr);

        duration_slider->setValue(5); // 2.5s

        QCOMPARE(duration_spin->value(), 2.5);
        QCOMPARE(mgr.config().duration, 2.5);

        auto saved = cfg_mgr.get_shake();
        QVERIFY(saved.has_value());
        QCOMPARE(saved->duration, 2.5);

        mgr.stop();
        std::filesystem::remove_all(dir);
    }

    void test_scale_slider_updates_manager_and_config()
    {
        const std::string dir = temp_dir() + "_scale";
        std::filesystem::remove_all(dir);
        setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

        ConfigManager cfg_mgr;
        cfg_mgr.load();

        ShakeManager mgr;
        mgr.start();
        ShakePanel panel(&mgr, &cfg_mgr);

        auto* scale_slider = panel.findChild<QSlider*>("shake_scale_slider");
        auto* scale_spin = panel.findChild<QDoubleSpinBox*>("shake_scale_spin");

        QVERIFY(scale_slider != nullptr);
        QVERIFY(scale_spin != nullptr);

        scale_slider->setValue(10); // 5.0x

        QCOMPARE(scale_spin->value(), 5.0);
        QCOMPARE(mgr.config().scale, 5.0);

        auto saved = cfg_mgr.get_shake();
        QVERIFY(saved.has_value());
        QCOMPARE(saved->scale, 5.0);

        mgr.stop();
        std::filesystem::remove_all(dir);
    }

    void test_badge_message_is_generic_when_unavailable()
    {
        const std::string dir = temp_dir() + "_badge";
        std::filesystem::remove_all(dir);
        setenv("XDG_CONFIG_HOME", dir.c_str(), 1);

        ConfigManager cfg_mgr;
        cfg_mgr.load();

        ShakeManager mgr;
        ShakePanel panel(&mgr, &cfg_mgr);

        QMetaObject::invokeMethod(&panel, "onAvailabilityChanged",
                                  Qt::DirectConnection,
                                  Q_ARG(bool, false));

        auto* badge = panel.findChild<QLabel*>("shake_badge_label");
        QVERIFY(badge != nullptr);
        QVERIFY(!badge->isHidden());
        QVERIFY(badge->text().contains("temporarily unavailable"));

        std::filesystem::remove_all(dir);
    }
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);

    TestShakePanel tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_shake_panel.moc"
