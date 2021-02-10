#pragma once
#include <utility>
#include <limits>
#include <cstdint>
#include <vector>
#include <stdexcept>

template<typename T>
std::pair<std::vector<double>, std::vector<double>> findMinMaxValues(
    const std::vector<T>& data, 
    const std::uint8_t componentSize
) {
    if (componentSize == 0) {
        throw std::logic_error("componentSize must be > 0");
    }

    if (data.empty()) {
        throw std::logic_error("data array cannot be empty");
    }
    
    if (data.size() % componentSize != 0) {
        throw std::runtime_error("data.size() % componentSize must equal 0");
    }

    std::vector<double> min(componentSize);
    std::vector<double> max(componentSize);
    std::fill(min.begin(), min.end(), std::numeric_limits<double>::infinity());
    std::fill(max.begin(), max.end(), -std::numeric_limits<double>::infinity());
    for (std::size_t i = 0; i < data.size(); i += componentSize) {
        for (std::size_t k = 0; k < componentSize; ++k) {
            min[k] = std::min(min[k], static_cast<double>(data.at(i + k)));
            max[k] = std::max(max[k], static_cast<double>(data.at(i + k)));
        }
    }

    return { min, max };
}