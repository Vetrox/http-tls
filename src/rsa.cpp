#include "rsa.h"
#include <iostream>
#include <string>

UnsignedBigInt encrypt_block(UnsignedBigInt message, UnsignedBigInt const& public_exponent, UnsignedBigInt const& modulus) {
    if (message >= modulus) {
        std::cout << "ERROR: Message was bigger than the modulus" << std::endl;
        abort();
    }
    return message.expmod(public_exponent, modulus);
}

UnsignedBigInt decrypt_block(UnsignedBigInt encrypted, UnsignedBigInt const& private_exponent, UnsignedBigInt const& modulus) {
    if (encrypted >= modulus) {
        std::cout << "ERROR: Encrypted message was bigger than the modulus" << std::endl;
        abort();
    }
    return encrypted.expmod(private_exponent, modulus);
}

std::vector<UnsignedBigInt> encrypt(std::vector<uint8_t> const& message, UnsignedBigInt const& public_exponent, UnsignedBigInt const& modulus) {
    auto block_amount = 0;
    for (int i = 0;; i++) {
        auto octets_levels = UnsignedBigInt(1);
        octets_levels <<= i*8; // TODO: case for log8(modulus)
        if (octets_levels < modulus) {
            block_amount = i;
        } else break;
    }

    std::cout << "BLOCK_AMOUNT: " << std::to_string(block_amount) << std::endl;
    
    if (block_amount == 0) {
        std::cout << "ERROR: Modulus is to small (less than 256)" << std::endl;
    }

    auto vec = std::vector<UnsignedBigInt>();
    auto msg_i = 0;

    while (msg_i < message.size()) {
        auto msg = UnsignedBigInt(0);
        for (int i = 0; i < block_amount && msg_i < message.size(); msg_i++, i++) {
            msg <<= 8;
            msg += UnsignedBigInt(message.at(message.size() - msg_i - 1));
            if (msg >= modulus) {
                std::cout << "WHY: " << std::to_string(i) << std::endl;
            }
        }
        vec.emplace_back(encrypt_block(std::move(msg), public_exponent, modulus));
    }
    return vec;
} 

std::vector<uint8_t> decrypt(std::vector<UnsignedBigInt> const& encrypted, UnsignedBigInt const& private_exponent, UnsignedBigInt const& modulus) {
    auto block_amount = 0;
    for (int i = 1;; i++) {
        auto octets_levels = UnsignedBigInt(1);
        octets_levels <<= i*8; // TODO: case for log2(modulus)
        if (octets_levels < modulus) {
            block_amount = i;
        } else break;
    }
    
    if (block_amount == 0) {
        std::cout << "ERROR: Modulus is to small (less than 256)" << std::endl;
    }

    std::vector<uint8_t> ret;
    for (auto const& encrypted_value : encrypted) {
        auto decrytped_value = decrypt_block(encrypted_value, private_exponent, modulus);
        for (int i = 0; i < block_amount; i++) {
            char octet = 0;
            for (int j = 0; j < 8; j++) {
                auto offset = i * 8 + j;
                if (decrytped_value.is_bit_set(offset)) {
                    octet |= 1 << j;
                }
            }
            ret.push_back(octet);
        }
    }
    return ret;
} 
