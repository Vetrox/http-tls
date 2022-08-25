#pragma once

#include <vector>
#include <string>

class BigInt {
public:
    BigInt(std::vector<uint8_t> data, bool is_positive) 
        : m_data(data) 
        , m_is_positive(is_positive)
    {
        ensure_minimum_data_size();
    }
    
    std::string as_binary() const;
    std::string as_decimal() const;

    void add(BigInt& other);
    void sub(BigInt& other);
    void mul(BigInt& other);
    void divmod(BigInt& other, BigInt& out_div, BigInt& out_mod) const;
    void shift_left(size_t);
    void shift_right(size_t);
    void set_bit(int, bool);

    void operator=(BigInt& other);
    void operator=(BigInt&& other);

    bool is_positive() const { return m_is_positive; }
    void set_is_positive(bool is_positive) { m_is_positive = is_positive; }

    std::strong_ordering operator<=>(BigInt const& other) const;
private:
    std::vector<uint8_t> m_data {}; // little endian (least significant byte first)
    bool m_is_positive {false};
    
    void ensure_minimum_data_size();
    void add_assume_both_positive(BigInt& other);
    void sub_ordered(BigInt& other);
};
