#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

inline bool isLittleEndian() {
    std::uint16_t n = 1;
    return (*(std::uint8_t*)&n == 1);
}

template <typename T>
inline std::vector<std::uint8_t>
primitiveVectorToLittleEndianByteVector(const std::vector<T>& input) noexcept {
    constexpr size_t byteSize = sizeof(T);
    std::vector<std::uint8_t> output(input.size() * byteSize);

    if (isLittleEndian()) {
        const auto* p = reinterpret_cast<const std::uint8_t*>(input.data());
        std::memcpy(output.data(), p, output.size());
        return output;
    }

    std::vector<std::uint8_t> tmp(byteSize);
    std::size_t byteIndex = 0;
    for (auto i = 0ul; i < input.size(); ++i) {
        std::memcpy(tmp.data(), input.data() + i, byteSize);
        auto src = byteSize - 1;
        for (auto dst = byteIndex; dst < (byteIndex + byteSize); ++dst, --src) {
            output[dst] = tmp[src];
        }
        byteIndex += byteSize;
    }

    return output;
}