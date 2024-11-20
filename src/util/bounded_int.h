// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BOUNDED_INT_H
#define BITCOIN_UTIL_BOUNDED_INT_H

#include <algorithm>
#include <concepts>
#include <limits>
#include <stdexcept>
#include <tinyformat.h>

template <typename T>
concept Integer = std::integral<T>;

/**
 * @brief Represents an integer constrained to the range [`Min`, `Max`].
 *
 * The constructor and assignment operator throw a `std::out_of_range`
 * exception for out-of-bounds values. The `clamped` method creates a
 * new `BoundedInt` with the value adjusted to fit within bounds, while
 * `clamp` can be used as a clamped alternative to the assignment
 * operator.
 *
 * @tparam T   The integer type (e.g., `int`, `long`).
 * @tparam Min The minimum allowed value (inclusive).
 * @tparam Max The maximum allowed value (inclusive).
 */
template <Integer T, T Min, T Max>
    requires(
        Min <= Max &&
        Min >= std::numeric_limits<T>::min() &&
        Max <= std::numeric_limits<T>::max())
class BoundedInt
{
public:
    constexpr BoundedInt(T value) : m_value{CheckBounds(value)} {}
    constexpr BoundedInt& operator=(T value)
    {
        m_value = CheckBounds(value);
        return *this;
    }
    static constexpr BoundedInt clamped(T value)
    {
        return BoundedInt{std::clamp(value, Min, Max)};
    }
    constexpr void clamp(T value)
    {
        m_value = std::clamp(value, Min, Max);
    }

    constexpr T value() const { return m_value; }
    constexpr operator T() const { return m_value; }

private:
    T m_value;
    // Throws if the value is out of bounds
    static constexpr T CheckBounds(T value)
    {
        if (value < Min || value > Max) {
            throw std::out_of_range(tfm::format("Value %i is out of bounds [%i, %i]", value, Min, Max));
        }
        return value;
    }
};

#endif // BITCOIN_UTIL_BOUNDED_INT_H
