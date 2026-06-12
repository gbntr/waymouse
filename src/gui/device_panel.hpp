#pragma once

#include "backends/backend.hpp"
#include <QWidget>

namespace waymouse {

class DevicePanel : public QWidget {
    Q_OBJECT

public:
    explicit DevicePanel(QWidget* parent = nullptr);
    ~DevicePanel() override;

    void setConfig(const Config& cfg);
    Config config() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace waymouse
