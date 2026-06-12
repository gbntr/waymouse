#pragma once

#include <string>
#include <memory>
#include <vector>

namespace waymouse {

struct Device {
    std::string name;
    std::string sysfs_path;
    std::string dev_node;
    std::string vendor_id;
    std::string product_id;
};

struct Config {
    double accel_speed = 0.0;
    std::string accel_profile = "default"; // default, flat, adaptive
    bool natural_scroll = false;
    bool left_handed = false;
};

class Backend {
public:
    virtual ~Backend() = default;
    virtual bool apply(const Device& device, const Config& cfg) = 0;
    virtual bool supports(const Device& device) const = 0;
    virtual std::string name() const = 0;
};

using BackendPtr = std::unique_ptr<Backend>;

} // namespace waymouse
