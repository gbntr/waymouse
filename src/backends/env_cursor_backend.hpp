#pragma once

#include "backends/cursor_backend.hpp"

namespace waymouse {

class EnvCursorBackend : public CursorBackend
{
public:
    bool apply_cursor_theme(const std::string& theme, int size) override;
    bool supports_runtime_cursor_change() const override;
    std::string name() const override;

private:
    void write_environment_d_file(const std::string& theme, int size) const;
};

} // namespace waymouse
