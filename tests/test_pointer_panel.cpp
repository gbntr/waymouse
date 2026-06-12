#include "gui/pointer_panel.hpp"
#include "core/pointer_manager.hpp"
#include "core/theme_detector.hpp"
#include "backends/cursor_backend.hpp"
#include <QApplication>
#include <QTest>
#include <QSignalSpy>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <string>
#include <vector>

using namespace waymouse;

// Mock CursorBackend
class MockCursorBackend : public CursorBackend
{
public:
    bool apply_cursor_theme(const std::string&, int) override { return m_runtime_support; }
    bool supports_runtime_cursor_change() const override { return m_runtime_support; }
    std::string name() const override { return "mock"; }

    bool m_runtime_support = false;
};

// Mock ThemeDetector
class MockThemeDetector : public ThemeDetector
{
public:
    std::vector<CursorTheme> detect_themes() const
    {
        CursorTheme t1{"Adwaita", "/usr/share/icons/Adwaita", false};
        CursorTheme t2{"Bibata", "/home/user/.icons/Bibata", true};
        return {t1, t2};
    }
};

class TestPointerPanel : public QObject
{
    Q_OBJECT

private:
    // Helper: find a child widget by type and optional text
    template<typename T>
    T* findChildWidget(QWidget* parent)
    {
        return parent->findChild<T*>();
    }

private slots:
    void test_panel_contains_widgets()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);
        PointerPanel panel(&mgr);

        // Verify key widgets exist
        QComboBox* combo = panel.findChild<QComboBox*>();
        QVERIFY(combo != nullptr);
        QVERIFY(combo->count() > 0);

        QSlider* slider = panel.findChild<QSlider*>();
        QVERIFY(slider != nullptr);
        QCOMPARE(slider->minimum(), 16);
        QCOMPARE(slider->maximum(), 64);

        QSpinBox* spinbox = panel.findChild<QSpinBox*>();
        QVERIFY(spinbox != nullptr);
        QCOMPARE(spinbox->minimum(), 16);
        QCOMPARE(spinbox->maximum(), 64);
        QCOMPARE(spinbox->singleStep(), 2);

        QPushButton* reset_btn = panel.findChild<QPushButton*>();
        QVERIFY(reset_btn != nullptr);

        QList<QLabel*> labels = panel.findChildren<QLabel*>();
        QVERIFY(labels.size() >= 2); // preview label + badge label
    }

    void test_theme_combobox_has_items()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);
        PointerPanel panel(&mgr);

        QComboBox* combo = panel.findChild<QComboBox*>();
        QVERIFY(combo != nullptr);

        // Should have 2 themes from mock
        QCOMPARE(combo->count(), 2);

        QStringList items;
        for (int i = 0; i < combo->count(); ++i)
            items << combo->itemText(i);

        QVERIFY(items.contains("Adwaita"));
        QVERIFY(items.contains("Bibata"));
    }

    void test_slider_syncs_with_spinbox()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);
        PointerPanel panel(&mgr);

        QSlider* slider = panel.findChild<QSlider*>();
        QSpinBox* spinbox = panel.findChild<QSpinBox*>();

        // Change slider and verify spinbox follows
        slider->setValue(32);
        QCOMPARE(spinbox->value(), 32);

        // Change spinbox and verify slider follows
        spinbox->setValue(48);
        QCOMPARE(slider->value(), 48);
    }

    void test_theme_change_updates_manager()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);
        PointerPanel panel(&mgr);

        QComboBox* combo = panel.findChild<QComboBox*>();

        // Select "Bibata" (index 1 in mock list)
        combo->setCurrentIndex(1);

        QCOMPARE(mgr.theme(), std::string("Bibata"));
    }

    void test_size_change_updates_manager()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);
        PointerPanel panel(&mgr);

        QSlider* slider = panel.findChild<QSlider*>();
        slider->setValue(32);

        QCOMPARE(mgr.size(), 32);
    }

    void test_reset_button_restores_defaults()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        PointerManager mgr(&detector, &backend);

        // Set non-default values
        mgr.set_theme("Bibata");
        mgr.set_size(48);

        PointerPanel panel(&mgr);

        QPushButton* reset_btn = panel.findChild<QPushButton*>();
        QVERIFY(reset_btn != nullptr);

        reset_btn->click();

        QCOMPARE(mgr.theme(), std::string(""));
        QCOMPARE(mgr.size(), 24);
    }

    void test_badge_visible_when_runtime_not_supported()
    {
        MockThemeDetector detector;
        MockCursorBackend backend;
        backend.m_runtime_support = false;
        PointerManager mgr(&detector, &backend);
        PointerPanel panel(&mgr);

        // Find all labels — badge should be visible
        QList<QLabel*> labels = panel.findChildren<QLabel*>();
        QLabel* badge = nullptr;
        for (auto* lbl : labels)
        {
            if (lbl->text().contains("Requer"))
            {
                badge = lbl;
                break;
            }
        }
        QVERIFY(badge != nullptr);
        QVERIFY(badge->isVisible());
    }
};

// We need a QApplication for widget tests
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);

    TestPointerPanel tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_pointer_panel.moc"
