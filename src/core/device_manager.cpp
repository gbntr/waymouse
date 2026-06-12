#include "core/device_manager.hpp"
#include <libudev.h>
#include <iostream>

namespace waymouse {

DeviceManager::DeviceManager()
{
}

std::vector<Device> DeviceManager::enumerate() const
{
    std::vector<Device> devices;

    struct udev* udev = udev_new();
    if (!udev)
        return devices;

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry* entries = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry = nullptr;

    udev_list_entry_foreach(entry, entries)
    {
        const char* path = udev_list_entry_get_name(entry);
        struct udev_device* dev = udev_device_new_from_syspath(udev, path);
        if (!dev)
            continue;

        const char* mouse = udev_device_get_property_value(dev, "ID_INPUT_MOUSE");
        const char* pointer = udev_device_get_property_value(dev, "ID_INPUT_POINTER");
        if ((mouse && std::string(mouse) == "1") || (pointer && std::string(pointer) == "1"))
        {
            Device d;
            const char* name = udev_device_get_property_value(dev, "NAME");
            d.name = name ? name : "Unknown";
            d.sysfs_path = path ? path : "";
            const char* devnode = udev_device_get_devnode(dev);
            d.dev_node = devnode ? devnode : "";
            const char* vendor = udev_device_get_property_value(dev, "ID_VENDOR_ID");
            const char* product = udev_device_get_property_value(dev, "ID_MODEL_ID");
            d.vendor_id = vendor ? vendor : "";
            d.product_id = product ? product : "";
            devices.push_back(std::move(d));
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return devices;
}

bool DeviceManager::is_pointer_device(const std::string& /*sysfs_path*/) const
{
    return true;
}

} // namespace waymouse
