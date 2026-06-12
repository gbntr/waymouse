#pragma once

#include "backends/backend.hpp"
#include <vector>

namespace waymouse {

class DeviceManager {
public:
    DeviceManager();

    std::vector<Device> enumerate() const;

private:
    bool is_pointer_device(const std::string& sysfs_path) const;
};

} // namespace waymouse
