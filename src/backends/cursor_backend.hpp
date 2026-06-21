#pragma once

#include <memory>
#include <string>

namespace waymouse {

class CursorBackend
{
public:
    virtual ~CursorBackend() = default;

    virtual bool apply_cursor_theme(const std::string& theme, int size) = 0;
    virtual bool supports_runtime_cursor_change() const = 0;
    virtual std::string name() const = 0;
};

using CursorBackendPtr = std::unique_ptr<CursorBackend>;

} // namespace waymouse
