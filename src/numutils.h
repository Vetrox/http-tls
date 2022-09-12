#pragma once

#include <vector>
#include <string>
#include <iostream>

class UnsignedBigInt {
public:
    UnsignedBigInt() {}

    UnsignedBigInt(std::vector<uint8_t> const data)
        : m_data(data)
    {
        ensure_minimum_data_size();
    }

    UnsignedBigInt(std::string_view number) {
        UnsignedBigInt temp;
        for (size_t i = 0; i < number.size(); i++) {
            if (number.at(i) > 9 + '0' || number.at(i) < '0') {
                std::cout << "malformed input" << std::endl;
                abort();
            }
            size_t digit = static_cast<size_t>(number.at(i) - '0');
            temp *= 10;
            temp += UnsignedBigInt(digit);
        }

        *this = temp;
    }

    UnsignedBigInt(size_t number)
    {
        for (size_t i = 0; i < sizeof(size_t); i++) {
            m_data.push_back((uint8_t) (number >> i*8));
        }
        ensure_minimum_data_size();
    }

    UnsignedBigInt(UnsignedBigInt const& old_obj)
        : m_data(old_obj.m_data)
    {
        ensure_minimum_data_size();
    }

    UnsignedBigInt(UnsignedBigInt const&& old_obj)
        : m_data(std::move(old_obj.m_data))
    {
        ensure_minimum_data_size();
    }

    std::string as_binary() const;
    std::string as_decimal() const;

    UnsignedBigInt operator+(UnsignedBigInt const& other) const;
    UnsignedBigInt operator-(UnsignedBigInt const& other) const;
    UnsignedBigInt operator*(UnsignedBigInt const& other) const;
    void operator+=(UnsignedBigInt const& other) {
        *this = *this + other;
    }
    void operator-=(UnsignedBigInt const& other) {
        *this = *this - other;
    }
    void operator*=(UnsignedBigInt const& other) {
        *this = *this * other;
    }
    void divmod(UnsignedBigInt const& other, UnsignedBigInt& out_div, UnsignedBigInt& out_mod) const;
    UnsignedBigInt operator/(UnsignedBigInt const& other) const;
    UnsignedBigInt operator%(UnsignedBigInt const& other) const;
    void operator/=(UnsignedBigInt const& other) {
        *this = *this / other;
    }
    void operator%=(UnsignedBigInt const& other) {
        *this = *this % other;
    }
    void operator<<=(size_t amount);
    void shift_right(size_t);
    void set_bit(size_t, bool);
    bool is_bit_set(size_t) const;
    UnsignedBigInt expmod(UnsignedBigInt const& exp, UnsignedBigInt const& mod) const;

    void operator=(UnsignedBigInt const& other);
    void operator=(UnsignedBigInt const&& other);

    std::strong_ordering operator<=>(UnsignedBigInt const& other) const;
    bool operator==(UnsignedBigInt const& other) const {
        return (*this <=> other) == std::strong_ordering::equal;
    }
    bool operator!=(UnsignedBigInt const& other) const {
        return !(*this == other);
    }
private:
    std::vector<uint8_t> m_data {}; // little endian (least significant byte first)

    void ensure_minimum_data_size();
};
