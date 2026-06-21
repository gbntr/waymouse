#include "core/mango_ipc_client.hpp"

#include <QProcess>
#include <QStandardPaths>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <utility>

namespace waymouse {

using json = nlohmann::json;

MangoIpcClient::MangoIpcClient() = default;

bool MangoIpcClient::is_available() const
{
    const char* signature = std::getenv("MANGO_INSTANCE_SIGNATURE");
    return signature != nullptr
        && *signature != '\0'
        && !QStandardPaths::findExecutable("mmsg").isEmpty();
}

std::vector<MonitorLayout> MangoIpcClient::query_monitor_layouts(std::string* error) const
{
    if (error)
        error->clear();

    const char* signature = std::getenv("MANGO_INSTANCE_SIGNATURE");
    if (!signature || !*signature)
    {
        if (error)
            *error = "MANGO_INSTANCE_SIGNATURE not set";
        return {};
    }

    const QString executable = QStandardPaths::findExecutable("mmsg");
    if (executable.isEmpty())
    {
        if (error)
            *error = "mmsg not found in PATH";
        return {};
    }

    QProcess proc;
    proc.start(executable, {"get", "all-monitors"});
    if (!proc.waitForFinished(2000))
    {
        if (error)
            *error = "mmsg timed out";
        proc.kill();
        return {};
    }

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
    {
        if (error)
            *error = proc.readAllStandardError().toStdString();
        return {};
    }

    const std::string output = proc.readAllStandardOutput().toStdString();
    auto layouts = parse_output(output);
    if (layouts.empty() && error && error->empty())
        *error = "mmsg returned no monitor layouts";
    return layouts;
}

std::optional<MonitorLayout> MangoIpcClient::focused_monitor(std::string* error) const
{
    auto layouts = query_monitor_layouts(error);
    for (const auto& layout : layouts)
    {
        if (layout.focused)
            return layout;
    }

    if (!layouts.empty())
        return layouts.front();
    return std::nullopt;
}

std::vector<MonitorLayout> MangoIpcClient::parse_output(const std::string& output)
{
    std::vector<MonitorLayout> layouts;

    try
    {
        auto payload = json::parse(output);
        const json* array = &payload;
        if (payload.contains("monitors"))
            array = &payload.at("monitors");

        if (!array->is_array())
            return layouts;

        for (const auto& item : *array)
        {
            MonitorLayout layout;
            layout.name = item.value("name", std::string{});
            layout.x = item.value("x", 0);
            layout.y = item.value("y", 0);
            layout.width = item.value("width", 0);
            layout.height = item.value("height", 0);
            layout.scale = item.value("scale", 1.0);
            layout.focused = item.value("focused", false);
            layouts.push_back(std::move(layout));
        }
        return layouts;
    }
    catch (...)
    {
        // Fall through to a tiny permissive parser for line-based output.
    }

    std::size_t pos = 0;
    while (pos < output.size())
    {
        std::size_t end = output.find('\n', pos);
        std::string line = output.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        pos = (end == std::string::npos) ? output.size() : end + 1;

        if (line.empty())
            continue;

        MonitorLayout layout;
        layout.name = line;
        layout.focused = layouts.empty();
        layouts.push_back(std::move(layout));
    }

    return layouts;
}

} // namespace waymouse
