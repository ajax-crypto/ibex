#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <cstring>

namespace ibex {

// UTF-8 string view - wraps std::string_view for UTF-8 data
// All strings in the compiler are assumed to be valid UTF-8
class Str {
public:
    // Empty string
    constexpr Str() = default;

    // From null-terminated string
    Str(const char* s) : data_(s, std::strlen(s)) {}

    // From std::string_view
    constexpr Str(std::string_view sv) : data_(sv) {}

    // From pointer and length
    constexpr Str(const char* ptr, size_t len) : data_(ptr, len) {}

    // Access data
    constexpr const char* ptr() const { return data_.data(); }
    constexpr size_t len() const { return data_.size(); }
    constexpr const char* data() const { return data_.data(); }
    constexpr size_t size() const { return data_.size(); }
    constexpr std::string_view view() const { return data_; }

    // Comparisons
    constexpr bool operator==(Str other) const { return data_ == other.data_; }
    constexpr bool operator!=(Str other) const { return data_ != other.data_; }

    // Check if empty
    constexpr bool is_empty() const { return data_.empty(); }

    // Implicit conversion to string_view
    constexpr operator std::string_view() const { return data_; }

private:
    std::string_view data_;
};

// Span of UTF-8 strings (for arrays of strings)
using StrSpan = std::span<const Str>;

// Mutable span of bytes (for building strings)
using ByteSpan = std::span<uint8_t>;

// Const span of bytes (for reading data)
using ConstByteSpan = std::span<const uint8_t>;

}  // namespace ibex
