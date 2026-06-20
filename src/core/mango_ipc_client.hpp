#pragma once

#include <optional>
#include <string>
#include <vector>

namespace waymouse {

struct MonitorLayout
{
    std::string name;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    double scale = 1.0;
    bool focused = false;
};

class MangoIpcClient
{
public:
    MangoIpcClient();

    bool is_available() const;
    std::vector<MonitorLayout> query_monitor_layouts(std::string* error = nullptr) const;
    std::optional<MonitorLayout> focused_monitor(std::string* error = nullptr) const;

private:
    static std::vector<MonitorLayout> parse_output(const std::string& output);
};

} // namespace waymouse
