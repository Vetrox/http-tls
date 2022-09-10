#include "SHA256.h"
#include <vector>
#include <iostream>
#include <array>

#include <iomanip>

std::array<uint32_t, 8> sha256_hash(std::vector<uint8_t> const& data) {
    std::vector<bool> temp;
    for (auto const& octet : data) {
        for (int i = 7; i >= 0; i--) {
            temp.push_back(octet & (1 << i));
        }
    }
    return sha256_hash(std::move(temp));
}


std::string sha256_as_hex(std::array<uint32_t, 8> hash) {
    std::stringstream ret;
    for (auto const& h : hash)
        ret << std::hex << std::setfill('0') << std::setw(2) << h;
    return ret.str();
}

uint32_t rotr(uint32_t x, uint32_t n) { // FROM: https://blog.regehr.org/archives/1063
  return (x>>n) | (x<< (32-n));
}

inline uint32_t sha256_ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}
inline uint32_t sha256_maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}
inline uint32_t sha256_bsig0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}
inline uint32_t sha256_bsig1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}
inline uint32_t sha256_ssig0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x>>3);
}
inline uint32_t sha256_ssig1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x>>10);
}

static constexpr std::array<uint32_t, 64> sha256_K = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

std::vector<std::array<uint32_t, 16>> split_into_512_bit_blocks(std::vector<bool> const& message) {
    if (message.size() % 512 != 0 || message.size() == 0) {
        std::cout << "message malformed" << std::endl;
        abort();
    }
    std::vector<std::array<uint32_t, 16>> ret{};
    size_t m_i = 0;
    for (; m_i * 512 < message.size(); m_i++) {
        auto current_block = m_i * 512;
        std::array<uint32_t, 16> var{};
        for (size_t i = 0; i < 16; i++) {
            auto current_int = i * 32;
            uint32_t val = 0;
            for (size_t j = 0; j < 32; j++) {
                size_t offset = current_block + current_int + j;
                val |= ((uint32_t) (message.at(offset) != 0)) << (31-j);
            }
            var[i] = val;
        }
        ret.push_back(std::move(var));
    }

    if (m_i * 512 != message.size()) {
        std::cout << "programming error in prev loop" << std::endl;
        abort();
    }
    return ret;
}

std::array<uint32_t, 8> sha256_hash(std::vector<bool> const& message) {
    uint32_t W[64];
    std::array<uint32_t, 8> prev_hash = { // initial state H(0)
        0x6a09e667, 0xbb67ae85, 0x3c6ef372,
        0xa54ff53a, 0x510e527f, 0x9b05688c,
        0x1f83d9ab, 0x5be0cd19
    };

    uint32_t a, b, c, d, e, f, g, h; // working variables
    uint32_t t1, t2;

    std::vector<bool> padded = add_padding(message);
    std::vector<std::array<uint32_t, 16>> blocks = split_into_512_bit_blocks(padded);

    auto N = blocks.size();
    for (size_t i = 0; i < N; i++) {
        auto M = blocks.at(i);
        // 1. Prepare the message schedule W.
        for (size_t t = 0; t < 16; t++) {
            W[t] = M[t];
        }
        for (size_t t = 16; t < 64; t++) {
            W[t] = sha256_ssig1(W[t-2]) + W[t-7] + sha256_ssig0(W[t-15])+ W[t-16];
        }
        // 2. Initialize the working variables.
        a = prev_hash[0];
        b = prev_hash[1];
        c = prev_hash[2];
        d = prev_hash[3];
        e = prev_hash[4];
        f = prev_hash[5];
        g = prev_hash[6];
        h = prev_hash[7];
        // 3. Perform the main hash computation.
        for (size_t t = 0; t < 64; t++) {
            t1 = h + sha256_bsig1(e) + sha256_ch(e,f,g) + sha256_K[t] + W[t];
            t2 = sha256_bsig0(a) + sha256_maj(a,b,c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        // 4. Compute the intermediate hash value H(i).
        prev_hash[0] += a;
        prev_hash[1] += b;
        prev_hash[2] += c;
        prev_hash[3] += d;
        prev_hash[4] += e;
        prev_hash[5] += f;
        prev_hash[6] += g;
        prev_hash[7] += h;
    }
    return prev_hash;
}


size_t find_l_k_solution(size_t length) {
    for (size_t k = 0; k < 512; k++) {
        // L + 1 + K = 448 (mod 512)
        if (((length + 1 + k) % 512) == 448) {
            return k;
        }
    }

    std::cout << "unreachable" << std::endl;
    abort();
}

std::vector<bool> add_padding(std::vector<bool> message) {
    size_t bit_size = message.size();
    if (true) { // when message length is less than < 2^64. size_t can only be 2^64 -1
        message.push_back(true);
    }

    auto K = find_l_k_solution(bit_size);
    for (size_t i = 0; i < K; i++) {
        message.push_back(false);
    }

    for (int i = 63; i >= 0; i--) {
        message.push_back(bit_size & ((size_t) 1 << (size_t) i));
    }

    return message;
}

