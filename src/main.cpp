#include <QApplication>
#include "core/compositor_detector.hpp"
#include "core/device_manager.hpp"
#include "core/config_manager.hpp"
#include "core/theme_detector.hpp"
#include "core/pointer_manager.hpp"
#include "core/shake_manager.hpp"
#include "core/shake_session_runtime.hpp"
#include "gui/main_window.hpp"
#include <iostream>
#include <algorithm>
#include <string>

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

    const bool shake_runtime_mode = std::any_of(argv, argv + argc, [](const char* arg) {
        return std::string(arg) == "--shake-runtime";
    });

    if (shake_runtime_mode)
    {
        waymouse::ShakeSessionRuntime runtime(&cfg_mgr,
                                              &pointer_mgr,
                                              detector.compositor_name());
        if (!runtime.start())
            return 1;
        return app.exec();
    }

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

    // --- Shake to Find ---
    ShakeManager shake_mgr(&pointer_mgr);
    shake_mgr.set_compositor_name(detector.compositor_name());

    // Load saved shake config on startup (or apply defaults)
    auto saved_shake = cfg_mgr.get_shake();
    if (saved_shake)
    {
        shake_mgr.set_config(*saved_shake);
    }
    else
    {
        shake_mgr.set_config(ShakeConfig{});
    }

    // Persist normalized shake config on startup (covers invalid TOML values too)
    cfg_mgr.set_shake(shake_mgr.config());
    cfg_mgr.save();

    // Note: auto-save of [pointer] config is handled by MainWindow
    // via PointerManager::changed signal connection.

    waymouse::MainWindow window(&dev_mgr,
                                &cfg_mgr,
                                backend.get(),
                                &pointer_mgr,
                                &shake_mgr);
    window.show();

    int ret = app.exec();

    // Cleanup monitoring thread before exit
    shake_mgr.stop();

    return ret;
}
