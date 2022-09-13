#include "nu_big_int.h"

void BigInt::divmod(BigInt const& other, BigInt& out_div, BigInt& out_mod) const {
    out_div.m_is_negative = m_is_negative ^ other.m_is_negative;
    out_mod.m_is_negative = m_is_negative;
    m_uint.divmod(other.m_uint, out_div.m_uint, out_mod.m_uint);
}

std::string BigInt::as_binary() const {
    return (m_is_negative ? "-" : "") + m_uint.as_binary();
}

std::string BigInt::as_decimal() const {
    return (m_is_negative ? "-" : "") + m_uint.as_decimal();
}


BigInt BigInt::operator+(BigInt const& other) const {
    auto bigger_sign = m_uint == other.m_uint ? false :
        (m_uint > other.m_uint ? m_is_negative : other.m_is_negative);
    auto add = m_uint + other.m_uint;
    return BigInt(std::move(add), bigger_sign);

}
BigInt BigInt::operator-(BigInt const& other) const {
    bool bigger_sign = m_uint == other.m_uint ? false :
        (m_uint > other.m_uint ? m_is_negative : !other.m_is_negative);
    return BigInt(m_uint + other.m_uint, bigger_sign);
}

BigInt BigInt::operator*(BigInt const& other) const {
    bool res_sign = m_is_negative ^ other.m_is_negative;
    return BigInt(m_uint * other.m_uint, res_sign);
}

BigInt BigInt::operator/(BigInt const& other) const {
    BigInt div(0);
    BigInt mod(0);
    divmod(other, div, mod);
    return div;
}

BigInt BigInt::operator%(BigInt const& other) const {
    BigInt div(0);
    BigInt mod(0);
    divmod(other, div, mod);
    return mod;
}

void BigInt::operator=(BigInt const& other) {
    m_uint = other.m_uint;
    m_is_negative = other.m_is_negative;
}
void BigInt::operator=(BigInt const&& other) {
    m_uint = std::move(other.m_uint);
    m_is_negative = other.m_is_negative;
}

std::strong_ordering BigInt::operator<=>(BigInt const& other) const {
    if (!m_is_negative && other.m_is_negative) {
        return std::strong_ordering::greater;
    }
    if (m_is_negative && !other.m_is_negative) {
        return std::strong_ordering::less; 
    }
    auto ordering = m_uint <=> other.m_uint;
    if (ordering == std::strong_ordering::equal) {
        return ordering;
    }
    if (m_is_negative && other.m_is_negative) {
        if (ordering == std::strong_ordering::greater) {
            return std::strong_ordering::less;
        }
        return std::strong_ordering::greater;
    }
    return ordering;
}
