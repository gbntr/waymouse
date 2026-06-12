#pragma once

#include "backends/backend.hpp"

namespace waymouse {

class LibinputBackend : public Backend {
public:
    bool apply(const Device& device, const Config& cfg) override;
    bool supports(const Device& device) const override;
    std::string name() const override;
};

} // namespace waymouse
