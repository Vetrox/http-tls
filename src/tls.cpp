#include "tls.h"
#include <chrono>
#include "string.h"

template<> uint8_t msg_type<ClientHello>() { return 1; }
template<typename T> uint8_t msg_type() { return 255; }

TLSPlaintext<Handshake<ClientHello>> client_hello() {
    const auto p1 = std::chrono::system_clock::now(); 
    const uint32_t time = std::chrono::duration_cast<std::chrono::seconds>(
                   p1.time_since_epoch()).count();
    
    TLSPlaintext<Handshake<ClientHello>> tls;

    Handshake<ClientHello> handshake;
    handshake.body.random.gmt_unix_time = time;
    memcpy(tls.fragment, &handshake, sizeof(handshake));

    return tls;
}
