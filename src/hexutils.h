#pragma once

#include <string>

namespace std {
    std::string to_hex(size_t number, bool space_out = false);
    std::string to_hex(unsigned char number);
}
