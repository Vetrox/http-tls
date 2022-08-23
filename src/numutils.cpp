#include "numutils.h"

#include <iostream>

void BigInt::ensure_minimum_data_size() {
    // cleanup most significant null bytes. this is an invariant
    for (int i = m_data.size() -1; i >= 0; i--) {
        if (m_data[i] == 0) {
            m_data.pop_back();
        }
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
}


void BigInt::add(BigInt& other) {
    if ((m_is_positive && other.m_is_positive) || (!m_is_positive && !other.m_is_positive)) {
        add_assume_both_positive(other);
    } else {
        // TODO:
    }
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
        uint16_t o = b.m_data[b_i];
        uint16_t t = a.m_data[a_i];
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
    ensure_minimum_data_size();
}

void BigInt::sub(BigInt& other) {
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

    for (int i = m_data.size(); i >= 0; i++) {
        if (m_data[i] > other.m_data[i]) {
            return false;
        } else if(m_data[i] < other.m_data[i]) {
            return true;
        }
    }
    return true;
}
