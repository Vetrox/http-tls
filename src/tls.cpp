#include "tls.h"
#include <chrono>
#include "string.h"

TLSPlaintext client_hello() {
    const auto p1 = std::chrono::system_clock::now(); 
    const uint32_t time = std::chrono::duration_cast<std::chrono::seconds>(
                   p1.time_since_epoch()).count();
    
    TLSPlaintext tls;

    Handshake handshake;
    handshake.body.random.gmt_unix_time = time;
    handshake.length3 = sizeof(ClientHello);

    memcpy(tls.fragment, &handshake, sizeof(Handshake));

    return tls;
}
