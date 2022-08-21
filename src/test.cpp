#include "tls.h"
#include "http.h"

#include <iostream>


int main() {
    auto handshake = client_hello();

    uint8_t* b = (uint8_t*) (&handshake);

    std::cout << "Length: " << sizeof(handshake) << std::endl;

    for (int i = 0; i < sizeof(handshake); i++) {
        std::cout << std::to_string(b[i]) << " ";
    }
    std::cout << std::endl;

    request("137.248.1.81", b, sizeof(handshake)); // maybe consider the packing bytes. and don't send them

    return 0;
}
