#pragma once

#include "backends/cursor_backend.hpp"
#include <memory>

namespace waymouse {

class EnvCursorBackend;

class WlrCursorBackend : public CursorBackend
{
public:
    WlrCursorBackend();
    ~WlrCursorBackend() override;

    bool apply_cursor_theme(const std::string& theme, int size) override;
    bool supports_runtime_cursor_change() const override;
    std::string name() const override;

private:
    std::unique_ptr<EnvCursorBackend> m_env_backend;
};

} // namespace waymouse
