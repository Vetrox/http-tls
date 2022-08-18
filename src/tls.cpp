#include "tls.h"
#include <chrono>
#include "string.h"

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
