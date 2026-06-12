#include <QApplication>
#include "core/compositor_detector.hpp"
#include "core/device_manager.hpp"
#include "core/config_manager.hpp"
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

    auto backend = create_backend(detector.detect());
    DeviceManager dev_mgr;
    ConfigManager cfg_mgr;

    if (!cfg_mgr.load())
    {
        std::cerr << "Warning: could not load config from " << cfg_mgr.config_path() << "\n";
    }

    waymouse::MainWindow window(&dev_mgr, &cfg_mgr, backend.get());
    window.show();

    return app.exec();
}
