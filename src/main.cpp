#include <QApplication>
#include "core/compositor_detector.hpp"
#include "core/device_manager.hpp"
#include "core/config_manager.hpp"
#include "core/theme_detector.hpp"
#include "core/pointer_manager.hpp"
#include "gui/main_window.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("waymouse");
    app.setOrganizationName("waymouse");

    using namespace waymouse;

    CompositorDetector detector;
    std::cout << "Detected compositor: " << detector.compositor_name() << "\n";

    auto backend_type = detector.detect();
    auto backend = create_backend(backend_type);
    auto cursor_backend = create_cursor_backend(backend_type);

    DeviceManager dev_mgr;
    ConfigManager cfg_mgr;

    if (!cfg_mgr.load())
    {
        std::cerr << "Warning: could not load config from "
                  << cfg_mgr.config_path() << "\n";
    }

    ThemeDetector theme_detector;
    PointerManager pointer_mgr(&theme_detector, cursor_backend.get());

    // Load saved pointer config on startup
    auto saved_pointer = cfg_mgr.get_pointer();
    if (saved_pointer)
    {
        pointer_mgr.load_config(*saved_pointer);
    }
    else
    {
        // Use environment-detected defaults on first launch
        PointerConfig defaults;
        defaults.theme = theme_detector.default_theme();
        defaults.size = theme_detector.default_size();
        pointer_mgr.load_config(defaults);
    }

    // Note: auto-save of [pointer] config is handled by MainWindow
    // via PointerManager::changed signal connection.

    waymouse::MainWindow window(&dev_mgr,
                                &cfg_mgr,
                                backend.get(),
                                &pointer_mgr);
    window.show();

    return app.exec();
}
