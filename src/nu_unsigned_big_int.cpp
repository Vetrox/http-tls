#include "numutils.h"

#include <iostream>


bool UnsignedBigInt::is_bit_set(size_t position) const {
    size_t whole_octet_shift = position / 8;
    size_t sub_octet_shift = position % 8;

    if (m_data.size() <= whole_octet_shift) {
        return false;
    }

    return (m_data.at(whole_octet_shift) & (1 << sub_octet_shift)) != 0;
}

UnsignedBigInt UnsignedBigInt::operator*(UnsignedBigInt const& other) const {
    auto ret = UnsignedBigInt(0);

    auto c_other = other; // TODO: only because of bitshift

    for (size_t i = 0; i < m_data.size(); i++) {
        for (size_t j = 0; j < 8; j++) {
            c_other = other;
            size_t offset = i*8+j;
            if (is_bit_set(offset)) {
                c_other <<= offset;
                ret += c_other;
            }
        }
    }
    return ret;
}


UnsignedBigInt UnsignedBigInt::operator/(UnsignedBigInt const& other) const {
    UnsignedBigInt c_mod = UnsignedBigInt({});
    UnsignedBigInt c_div = UnsignedBigInt({});

    divmod(other, c_div, c_mod);

    return c_div;
}

UnsignedBigInt UnsignedBigInt::operator%(UnsignedBigInt const& other) const {
    UnsignedBigInt c_mod = UnsignedBigInt({});
    UnsignedBigInt c_div = UnsignedBigInt({});

    divmod(other, c_div, c_mod);

    return c_mod;
}
/** Fast exponentiation:
 *  calculates this = (this^z) % n
 *
 */
/*void UnsignedBigInt::expmod(UnsignedBigInt &z, UnsignedBigInt &n){
    auto a1 = this;
    auto z1 = z;
    auto x = 1;

    while (z1 != 0) {
        while ((z1 mod 2) == 0) {
            z1 = z1 / 2;
            a1 = (a1 * a1) mod n;
        }
        z1 = z1 - 1;
        x = (x * a1) mod n;
    }
}*/

/** Fast exponentiation:
 *  calculates this = (this^z) % n
 *
 */
UnsignedBigInt UnsignedBigInt::expmod(UnsignedBigInt const& z, UnsignedBigInt const& n) const {
    auto a1 = *this;
    auto z1 = z;
    auto x = UnsignedBigInt(1);


    auto two = UnsignedBigInt(2);
    auto const zero = UnsignedBigInt(0);

    while (z1 != zero) {
        while ((z1 % two) == zero) {
            z1 = z1 / two;
            a1 = (a1 * a1) % n;
        }
        z1 = z1 - 1;
        x = (x * a1) % n;
    }

    return x;
}

void UnsignedBigInt::operator=(UnsignedBigInt const&& other) {
    m_data = std::move(other.m_data);
    ensure_minimum_data_size();
}
void UnsignedBigInt::operator=(UnsignedBigInt const& other) {
    m_data = other.m_data;
    ensure_minimum_data_size();
}

void UnsignedBigInt::divmod(UnsignedBigInt const& divisor, UnsignedBigInt& out_div, UnsignedBigInt& out_mod) const {
    if (out_div != UnsignedBigInt(0) || out_mod != UnsignedBigInt(0)) {
        std::cout << "precondition for divmod not met: out_div and out_mod must be 0" << std::endl;
        abort();
    }
    out_mod.m_data = m_data;
    for (size_t oi = 0; oi < m_data.size(); oi++) {
        size_t i = m_data.size() - 1 - oi;
        for (size_t oj = 0; oj < 8; oj++) {
            size_t j = 7 - oj;
            size_t shift_amount = i * 8 + j; // TODO: prevent "overshifting"
            auto c_divisor = divisor;
            c_divisor <<= shift_amount;
            if (c_divisor <= out_mod) {
                out_mod -= c_divisor;
                out_div.set_bit(shift_amount, true);
            }
        }
    }
    out_div.ensure_minimum_data_size();
    out_mod.ensure_minimum_data_size();
}

void UnsignedBigInt::set_bit(size_t position, bool value) {
    size_t whole_octet_shift = position / 8;
    size_t sub_octet_shift = position % 8;

    // [mds-1,wos] == [mds-1,wos+1); amount = wos+1 - (mds - 1) = wos + 1 - mds + 1 = wos - mds + 2
    while (m_data.size() < whole_octet_shift + 1)
        m_data.push_back(0);


    uint8_t new_val = m_data.at(whole_octet_shift);
    if (value) {
        new_val |= static_cast<uint8_t>(1 << sub_octet_shift);
    } else {
        new_val &= static_cast<uint8_t>(~(1 << sub_octet_shift));
    }
    m_data[whole_octet_shift] = new_val;
    ensure_minimum_data_size();
}

void UnsignedBigInt::operator<<=(size_t amount) {
    size_t whole_octet_shift = amount / 8;
    size_t sub_octet_shift = amount % 8;

    uint8_t bit_mask = ~static_cast<uint8_t>(0xff >> sub_octet_shift);

    if (sub_octet_shift > 0) {
        m_data.push_back(0);
    }
    for (size_t i = 0; i < whole_octet_shift; i++) {
        m_data.push_back(0);
    }

    for (size_t j = 0; j < m_data.size(); j++) {
        size_t i = m_data.size() - 1 - j;
        size_t here = 0;
        if (i >= whole_octet_shift) {
            here = m_data[i-whole_octet_shift];
            here <<= sub_octet_shift;
            if (i >= whole_octet_shift + 1) {
                here |= static_cast<size_t>(m_data[i-whole_octet_shift-1] & bit_mask) >> (8 - sub_octet_shift);
            } // otherwise insert 0s at the location (which is already done.)
        }
        m_data[i] = static_cast<uint8_t>(here);
    }
    ensure_minimum_data_size();
}

void UnsignedBigInt::ensure_minimum_data_size() {
    // cleanup most significant null bytes. this is an invariant
    size_t original_size = m_data.size();
    for (size_t i = 0; i < original_size; i++) {
        if (m_data.at(original_size - 1 - i) != 0) return;
        m_data.pop_back();
    }
}

std::string UnsignedBigInt::as_decimal() const {
    std::string s = "";
    UnsignedBigInt div, mod;
    auto cache = UnsignedBigInt(m_data);
    auto ten = UnsignedBigInt(10);
    while (cache >= ten) {
        div = UnsignedBigInt(0);
        mod = UnsignedBigInt(0);
        cache.divmod(ten, div, mod);
        if (cache <= div) {
            // div didn't get smaller. (why??)
            std::cout << "div not smaller" << std::endl;
            abort();
        }
        cache = div;
        if (mod.m_data.size() == 0) {
            s = "0" + s;
        } else {
            s = std::to_string(mod.m_data[0]) + s;
        }
    }
    if (cache.m_data.size() == 0) {
        s = "0" + s;
    } else {
        s = std::to_string(cache.m_data[0]) + s;
    }
    return s;
}

std::string UnsignedBigInt::as_binary() const {
    std::string s = "";
    for (size_t i = 0; i < m_data.size(); i++) {
        uint8_t c = m_data[m_data.size() - 1 - i];
        for (int j = 7; j >= 0; j--) {
            bool is_set = ((c >> j) & 1);
            s += is_set ? "1" : "0";
        }
    }
    if (s.length() == 0) s += '0';
    return s;
}

UnsignedBigInt UnsignedBigInt::operator+(UnsignedBigInt const& other) const {
    size_t carry = 0;
    size_t other_i = 0;
    size_t our_i = 0;

    auto cache = std::vector<uint8_t>();

    for (; other_i < other.m_data.size() && our_i < m_data.size(); other_i++, our_i++) {
        size_t o = other.m_data[other_i];
        size_t t = m_data[our_i];
        size_t res = o + t + carry;
        carry = res >> 8;
        cache.push_back(static_cast<uint8_t>(res & 0xff));
    }

    for (; other_i < other.m_data.size(); other_i++) {
        size_t o = other.m_data[other_i];
        size_t res = o + carry;
        carry = res >> 8;
        cache.push_back((uint8_t) (res & 0xff));
    }

    for (; our_i < m_data.size(); our_i++) {
        size_t t = m_data[our_i];
        size_t res = t + carry;
        carry = res >> 8;
        cache.push_back((uint8_t) (res & 0xff));
    }

    if (carry > 0) {
        cache.push_back(1);
    }

    return UnsignedBigInt(std::move(cache));
}

UnsignedBigInt UnsignedBigInt::operator-(UnsignedBigInt const& other) const {
    if (other > (*this)) {
        std::cout << "UnsignedBigInt: '-' called but other was larger." << std::endl;
        abort();
    }
    // calculate this - other. other <= this

    std::vector<uint8_t> cache;
    uint8_t carry = 0;
    size_t this_i = 0;
    size_t other_i = 0;

    for (; other_i < other.m_data.size() && this_i < m_data.size(); other_i++, this_i++) {
        uint16_t t = m_data[this_i];
        uint16_t o = other.m_data[other_i];
        ssize_t res = t - o - carry;
        if (res < 0) {
            res += 0x100;
            carry = 1;
        } else {
            carry = 0;
        }
        cache.push_back((uint8_t) (res & 0xff)); // TODO: maybe strip the bitmask
    }

    if (other_i < other.m_data.size()) {
        std::cout << "ERROR: other.m_data was not consumed fully. other_i=" << other_i << " other.size()=" << other.m_data.size() << std::endl;
        abort();
    }

    for (; this_i < m_data.size(); this_i++) {
        uint16_t t = m_data[this_i];
        ssize_t res = t - carry;
        if (res < 0) {
            res += 0x100;
            carry = 1;
        } else {
            carry = 0;
        }
        cache.push_back((uint8_t) (res & 0xff)); // TODO: maybe strip the bitmask
    }

    if (carry > 0) {
        std::cout << "UNEXPECTED: Carry shouldn't be 1 here ever, no?" << std::endl;
        abort();
    }

    return UnsignedBigInt(cache);
}

std::strong_ordering UnsignedBigInt::operator<=>(UnsignedBigInt const& other) const {
    if (m_data.size() > other.m_data.size()) {
        return std::strong_ordering::greater;
    } else if (m_data.size() < other.m_data.size()) {
        return std::strong_ordering::less;
    }

    for (size_t i = 0; i < m_data.size(); i++) {
        auto our_data = m_data.at(static_cast<size_t>(m_data.size() - i - 1));
        auto other_data = other.m_data.at(static_cast<size_t>(m_data.size() - i - 1));
        if (our_data > other_data) {
            return std::strong_ordering::greater;
        } else if(our_data < other_data) {
            return std::strong_ordering::less;
        }
    }
    return std::strong_ordering::equal;
}
