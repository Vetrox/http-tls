#include "numutils.h"

#include <iostream>

void BigInt::divmod(BigInt& divisor, BigInt& out_div, BigInt& out_mod) const {
    // FIXME: ensure 0ed out_div and out_mod
    out_mod.m_data = m_data;
    std::cout << "divisor: " << divisor.as_binary() << std::endl;
    for (int i = m_data.size() - 1; i >= 0; i--) {
        for (int j = 7; j >= 0; j--) {
            auto shift_amount = i * 8 + j; // TODO: prevent "overshifting"
            auto c_divisor = BigInt(divisor.m_data, true); // FIXME: implement shift right
            c_divisor.shift_left(shift_amount);
            std::cout << "cdi: " << c_divisor.as_binary() << std::endl;
            std::cout << "mod: " << out_mod.as_binary() << std::endl;
            if (c_divisor.less_than_eq_abs(out_mod)) {
                std::cout << "leq true" << std::endl;
                out_mod.sub(c_divisor);
                out_div.set_bit(shift_amount, true);
            } else {
                std::cout << "leq false" << std::endl;
            }
        }
    }
    out_div.ensure_minimum_data_size();
    out_mod.ensure_minimum_data_size();
}

void BigInt::set_bit(int position, bool value) {
    int whole_octet_shift = position / 8;
    int sub_octet_shift = position % 8;

    int minimum_octets = whole_octet_shift + (sub_octet_shift > 0 ? 1 : 0);
    for (int i = 0; i < minimum_octets; i++) { // TODO: maybe add less
        m_data.push_back(0);
    }
    
    int new_val = m_data[whole_octet_shift];
    if (value) {
        new_val |= 1 << sub_octet_shift;
    } else {
        new_val &= ~(1 << sub_octet_shift);
    }
    m_data[whole_octet_shift] = new_val;
    ensure_minimum_data_size();
}

void BigInt::shift_left(size_t amount) {
    int whole_octet_shift = amount / 8;
    int sub_octet_shift = amount % 8;

    uint8_t bit_mask = ~(0xff >> sub_octet_shift);

    if (sub_octet_shift > 0) {
        m_data.push_back(0);
    }
    for (int i = 0; i < whole_octet_shift; i++) {
        m_data.push_back(0);
    }

    for (int i = m_data.size() - 1; i >= 0; i--) {
        uint8_t here = 0;
        if (i - whole_octet_shift >= 0) {
            here = m_data[i-whole_octet_shift];
            here <<= sub_octet_shift;
            if (i - whole_octet_shift - 1 >= 0) {
                here |= (m_data[i-whole_octet_shift-1] & bit_mask) >> (8 - sub_octet_shift);
            } // otherwise insert 0s at the location (which is already done.)
        }
        m_data[i] = here;
    }
    ensure_minimum_data_size();
}

void BigInt::ensure_minimum_data_size() {
    // cleanup most significant null bytes. this is an invariant
    for (int i = m_data.size() - 1; i >= 0; i--) {
        if (m_data[i] != 0) return;
        m_data.pop_back();
    }
}


std::string BigInt::as_binary() const {
    std::string s = "";
    for (size_t i = 0; i < m_data.size(); i++) {
        uint8_t c = m_data[m_data.size() - 1 - i];
        for (int j = 7; j >= 0; j--) {
            bool is_set = ((c >> j) & 1);
            s += is_set ? "1" : "0";
        }
    }
    return s;
}

void BigInt::add_assume_both_positive(BigInt& other) {
    uint8_t carry = 0;
    size_t other_i = 0;
    size_t our_i = 0;
    for (; other_i < other.m_data.size() && our_i < m_data.size(); other_i++, our_i++) {
        uint16_t o = other.m_data[other_i];
        uint16_t t = m_data[our_i];
        uint16_t res = o + t + carry;
        carry = res >> 8;
        m_data[our_i] = (uint8_t) (res & 0xff);
    }

    for (; other_i < other.m_data.size(); other_i++) {
        uint16_t o = other.m_data[other_i];
        uint16_t res = o + carry;
        carry = res >> 8;
        m_data.push_back((uint8_t) (res & 0xff));
    } 

    for (; our_i < m_data.size(); our_i++) {
        uint16_t t = m_data[our_i];
        uint16_t res = t + carry;
        carry = res >> 8;
        m_data[our_i] = (uint8_t) (res & 0xff);
    }

    if (carry > 0) {
        m_data.push_back(1);
    }
    ensure_minimum_data_size();
}


void BigInt::add(BigInt& other) {
    if ((m_is_positive && other.m_is_positive) || (!m_is_positive && !other.m_is_positive)) {
        add_assume_both_positive(other);
    } else {
        std::cout << "unimplemented yet" << std::endl;
        abort();
        // TODO:
    }
    // ensure maybe TODO
}

void BigInt::sub_ordered(BigInt& other) {
    auto compare_res = other.less_than_eq_abs(*this);
    auto& a = compare_res ? *this : other;
    auto& b = compare_res ? other : *this;
    // a - b. b <= a
    
    std::vector<uint8_t> temp;
    uint8_t carry = 0;
    size_t a_i = 0;
    size_t b_i = 0;
    for (; b_i < b.m_data.size() && a_i < a.m_data.size(); b_i++, a_i++) {
        uint16_t t = a.m_data[a_i];
        uint16_t o = b.m_data[b_i];
        int16_t res = t - o - carry;
        if (res < 0) {
            res += 0x100;
            carry = 1;
        } else {
            carry = 0;
        }
        temp.push_back((uint8_t) (res & 0xff));
    }

    if (b_i < b.m_data.size()) {
        std::cout << "ERROR: b.m_data was not consumed fully. b=" << b_i << " b.size()=" << b.m_data.size() << std::endl;
        abort();
    }

    for (; a_i < a.m_data.size(); a_i++) {
        uint16_t t = a.m_data[a_i];
        int16_t res = t - carry;
        if (res < 0) {
            res += 0x100;
            carry = 1;
        } else {
            carry = 0;
        }
        temp.push_back((uint8_t) (res & 0xff));
    }

    if (carry > 0) {
        std::cout << "UNEXPECTED: Carry shouldn't be 1 here ever, no?" << std::endl;
        abort();
    }
    
    m_data = temp;
    std::cout << "Updated: " << as_binary() << std::endl;
    ensure_minimum_data_size();
}

void BigInt::sub(BigInt& other) {
    std::cout << "sub entered" << std::endl;
    bool other_inv = !other.m_is_positive;

    if (m_is_positive && other_inv) {
        add_assume_both_positive(other);
    } else if(m_is_positive && !other_inv) {
        // a - b
        sub_ordered(other);
    } else if(!m_is_positive && other_inv) {
        // b - a
        sub_ordered(other);
    } else {
        // -b -a = - (a + b)
        add_assume_both_positive(other);
    }
}

bool BigInt::less_than_eq_abs(BigInt& other) const {
    if (m_data.size() > other.m_data.size()) {
        return false;
    } else if (m_data.size() < other.m_data.size()) {
        return true;
    }

    for (int i = m_data.size() -1; i >= 0; i--) {
        auto our_data = m_data.at(i);
        auto other_data = other.m_data.at(i);
        if (our_data > other_data) {
            return false;
        } else if(our_data < other_data) {
            return true;
        }
    }
    return true;
}
