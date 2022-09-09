#include <string>
#include <iomanip>

namespace std {
    std::string to_hex(size_t number, bool space_out = false) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < sizeof(size_t); i++) {
            if (space_out && i != 0) ss << ' ';
            ss << std::setw(2) << (number >> 8 * (7 - i) & 0xff);
        }
        return ss.str();
    }
    std::string to_hex(unsigned char number) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0')
            << std::setw(2) << +number;
        return ss.str();
    }
}
