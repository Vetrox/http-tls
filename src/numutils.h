#pragma once

#include <vector>
#include <string>

class BigInt {
public:
    BigInt(std::vector<uint8_t> const data, bool is_positive) 
        : m_data(data) 
        , m_is_positive(is_positive)
    {
        ensure_minimum_data_size();
    }

    BigInt(BigInt const& old_obj) 
        : m_data(old_obj.m_data)
        , m_is_positive(old_obj.m_is_positive) 
    {
        ensure_minimum_data_size();
    }
    
    BigInt(BigInt const&& old_obj) 
        : m_data(std::move(old_obj.m_data))
        , m_is_positive(old_obj.m_is_positive) 
    {
        ensure_minimum_data_size();
    }
    
    std::string as_binary() const;
    std::string as_decimal() const;

    void add(BigInt const& other);
    void sub(BigInt const& other);
    void mul(BigInt const& other);
    void divmod(BigInt const& other, BigInt& out_div, BigInt& out_mod) const;
    void operator/(BigInt const& other);
    void operator%(BigInt const& other);
    void shift_left(size_t);
    void shift_right(size_t);
    void set_bit(int, bool);
    void expmod(BigInt const& exp, BigInt const& mod);

    void operator=(BigInt const& other);
    void operator=(BigInt const&& other);

    bool is_positive() const { return m_is_positive; }
    void set_is_positive(bool is_positive) { m_is_positive = is_positive; }

    std::strong_ordering operator<=>(BigInt const& other) const;
    bool operator==(BigInt const& other) const {
        return (*this <=> other) == std::strong_ordering::equal;
    }
    bool operator!=(BigInt const& other) const {
        return !(*this == other);
    }
private:
    std::vector<uint8_t> m_data {}; // little endian (least significant byte first)
    bool m_is_positive {false};
    
    void ensure_minimum_data_size();
    void add_assume_both_positive(BigInt const& other);
    void sub_ordered(BigInt const& other);
};
