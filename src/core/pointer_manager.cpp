#include "core/pointer_manager.hpp"
#include <algorithm>

namespace waymouse {

PointerManager::PointerManager(ThemeDetector* detector,
                               CursorBackend* backend,
                               QObject* parent)
    : QObject(parent)
    , m_detector(detector)
    , m_backend(backend)
{
}

void PointerManager::set_theme(const std::string& theme)
{
    m_config.theme = theme;
    emit changed();
    apply();
}

void PointerManager::set_size(int size)
{
    m_config.size = normalize_size(size);
    emit changed();
    apply();
}

std::string PointerManager::theme() const
{
    return m_config.theme;
}

int PointerManager::size() const
{
    return m_config.size;
}

std::vector<CursorTheme> PointerManager::available_themes() const
{
    return m_detector->detect_themes();
}

bool PointerManager::can_apply_runtime() const
{
    return m_backend->supports_runtime_cursor_change();
}

void PointerManager::reset_to_defaults()
{
    m_config.theme.clear();
    m_config.size = 24;
    emit changed();
    apply();
}

void PointerManager::load_config(const PointerConfig& cfg)
{
    m_config.theme = cfg.theme;
    m_config.size = normalize_size(cfg.size);
    emit changed();
}

void PointerManager::apply()
{
    bool success = m_backend->apply_cursor_theme(m_config.theme, m_config.size);
    emit runtime_applied(success);
}

int PointerManager::normalize_size(int raw)
{
    // Step 1: Clamp to [16, 64]
    int clamped = std::clamp(raw, 16, 64);

    // Step 2: Round to nearest even (odd → round up)
    if (clamped % 2 != 0)
        clamped += 1;

    // Step 3: Ensure still in range after rounding
    // (only edge case: if raw was 65, clamped was 64 which is even)
    // But if raw was 15, clamped was 16 which is even
    // So no additional range check needed.

    return clamped;
}

} // namespace waymouse
