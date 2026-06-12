#pragma once

#include "core/pointer_config.hpp"
#include "core/theme_detector.hpp"
#include "backends/cursor_backend.hpp"
#include <QObject>
#include <memory>
#include <string>
#include <vector>

namespace waymouse {

class PointerManager : public QObject
{
    Q_OBJECT

public:
    explicit PointerManager(ThemeDetector* detector,
                            CursorBackend* backend,
                            QObject* parent = nullptr);

    void set_theme(const std::string& theme);
    void set_size(int size);
    std::string theme() const;
    int size() const;
    std::vector<CursorTheme> available_themes() const;
    bool can_apply_runtime() const;

    void reset_to_defaults();

    // Load config from config manager (used on startup)
    void load_config(const PointerConfig& cfg);

signals:
    void changed();
    void runtime_applied(bool success);

private:
    void apply();
    static int normalize_size(int raw);

    ThemeDetector* m_detector;
    CursorBackend* m_backend;
    PointerConfig m_config;
};

} // namespace waymouse
