#include "numutils.h"
#include <string>


std::vector<UnsignedBigInt> encrypt(std::vector<uint8_t> const& message, UnsignedBigInt const& public_exponent, UnsignedBigInt const& modulus);

std::vector<uint8_t> decrypt(std::vector<UnsignedBigInt> const& encrypted, UnsignedBigInt const& private_exponent, UnsignedBigInt const& modulus);
