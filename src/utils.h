#pragma once
#include <ranges>
#include <string>

template <std::ranges::input_range Range>
std::string join(const Range& range, const std::string& separator) {
    std::string result;
    bool first = true;
    for (const auto& item : range) {
        if (!first) {
            result += separator;
        }
        result += item;
        first = false;
    }
    return result;
}
