#include "sha256.h"
#include <vector>
#include <iostream>

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
    for (int i = 0; i < K; i++) {
        message.push_back(false);
    }

    for (int i = 63; i >= 0; i--) {
        message.push_back(bit_size & ((size_t) 1 << (size_t) i));
    }

    return message;
}

