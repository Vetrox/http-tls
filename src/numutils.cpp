#include "numutils.h"

#include <iostream>

bool UnsignedBigInt::is_bit_set(size_t position) const {
    int whole_octet_shift = position / 8;
    int sub_octet_shift = position % 8;

    if (m_data.size() < whole_octet_shift) {
        return false;
    }
    
    return (m_data.at(whole_octet_shift) & (1 << sub_octet_shift)) != 0;
}

UnsignedBigInt UnsignedBigInt::operator*(UnsignedBigInt const& other) const {
    auto ret = UnsignedBigInt(0);
    
    auto c_other = other; // TODO: only because of bitshift

    for (int i = 0; i < m_data.size(); i++) {
        for (int j = 0; j < 8; j++) {
            c_other = other;
            int offset = i*8+j;
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

    return std::move(c_div);
}

UnsignedBigInt UnsignedBigInt::operator%(UnsignedBigInt const& other) const {
    UnsignedBigInt c_mod = UnsignedBigInt({});
    UnsignedBigInt c_div = UnsignedBigInt({});
    
    divmod(other, c_div, c_mod);

    return std::move(c_mod);
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
    if (out_div.m_data.size() != 0 || out_mod.m_data.size() != 0) {
        std::cout << "precondition for divmod not met: out_div and out_mod must be 0" << std::endl;
        // TODO: implement 0 check and 0 number.
        abort();
    }
    out_mod.m_data = m_data;
    for (int i = m_data.size() - 1; i >= 0; i--) {
        for (int j = 7; j >= 0; j--) {
            auto shift_amount = i * 8 + j; // TODO: prevent "overshifting"
            auto c_divisor = UnsignedBigInt(divisor.m_data); // FIXME: implement shift right
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

void UnsignedBigInt::set_bit(int position, bool value) {
    int whole_octet_shift = position / 8;
    int sub_octet_shift = position % 8;

    for (int i = m_data.size() - 1; i <= whole_octet_shift; i++) {
        m_data.push_back(0);
    }
    
    int new_val = m_data.at(whole_octet_shift);
    if (value) {
        new_val |= 1 << sub_octet_shift;
    } else {
        new_val &= ~(1 << sub_octet_shift);
    }
    m_data[whole_octet_shift] = new_val;
    ensure_minimum_data_size();
}

void UnsignedBigInt::operator<<=(size_t amount) {
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

void UnsignedBigInt::ensure_minimum_data_size() {
    // cleanup most significant null bytes. this is an invariant
    for (int i = m_data.size() - 1; i >= 0; i--) {
        if (m_data[i] != 0) return;
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
    if (s.length() == 0) s = "0";
    return s;
}

UnsignedBigInt UnsignedBigInt::operator+(UnsignedBigInt const& other) const {
    uint8_t carry = 0;
    size_t other_i = 0;
    size_t our_i = 0;

    auto cache = std::vector<uint8_t>();

    for (; other_i < other.m_data.size() && our_i < m_data.size(); other_i++, our_i++) {
        uint16_t o = other.m_data[other_i];
        uint16_t t = m_data[our_i];
        uint16_t res = o + t + carry;
        carry = res >> 8;
        cache.push_back((uint8_t) (res & 0xff));
    }

    for (; other_i < other.m_data.size(); other_i++) {
        uint16_t o = other.m_data[other_i];
        uint16_t res = o + carry;
        carry = res >> 8;
        cache.push_back((uint8_t) (res & 0xff));
    } 

    for (; our_i < m_data.size(); our_i++) {
        uint16_t t = m_data[our_i];
        uint16_t res = t + carry;
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
        int16_t res = t - o - carry;
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
        int16_t res = t - carry;
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

    for (int i = m_data.size() - 1; i >= 0; i--) {
        auto our_data = m_data.at(i);
        auto other_data = other.m_data.at(i);
        if (our_data > other_data) {
            return std::strong_ordering::greater;
        } else if(our_data < other_data) {
            return std::strong_ordering::less;
        }
    }
    return std::strong_ordering::equal;
}
