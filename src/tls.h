#pragma once
#include "stdint.h"
#include <arpa/inet.h>


#pragma pack(push, 1)
struct Random {
    /* The current time and date in standard UNIX 32-bit format. */
    uint32_t gmt_unix_time;
    /* 28 bytes generated by a secure random number generator. */
    uint8_t random_bytes[28] = {0,0,0,0,5,0,0,0,0,0,3,0,0,1,0,0,0,0,0,0,0,0,0,0,0
    ,0,0,6};
};

struct ClientHello {
    /* The version of the TLS protocol by which the client wishes to
     * communicate during this session.  This SHOULD be the latest
     * (highest valued) version supported by the client.  For this
     * version of the specification, the version will be 3.3  */
    uint8_t client_version[2] {3, 3};

    Random random;
    /* The ID of a session the client wishes to use for this connection.
     * This field is empty if no session_id is available, or if the
     * client wishes to generate new security parameters. */
    uint8_t session_id_length = 0;
    uint8_t session_id[0]; // <0..32> meaning 0 to 32 bytes.
    uint16_t cipher_suites_length = htons(34);
    uint8_t cipher_suites [34] = { 
        0x13, 0x01,
        0x13, 0x03,
        0x13, 0x02,
        0xc0, 0x2b,
        0xc0, 0x2f,
        0xcc, 0xa9,
        0xcc, 0xa8,
        0xc0, 0x2c,
        0xc0, 0x30,
        0xc0, 0x0a,
        0xc0, 0x09,
        0xc0, 0x13,
        0xc0, 0x14,
        0x00, 0x9c,
        0x00, 0x9d,
        0x00, 0x2f,
        0x00, 0x35
    }; // todo https://www.rfc-editor.org/rfc/rfc5246#appendix-A.5
    uint8_t compression_methods_length = 1;
    uint8_t compression_methods[1] = { 0x00 }; // no compression
    uint16_t extensions_length = 0;
};


struct ServerHello {
    uint8_t server_version[2];
    Random random;
    uint8_t session_id_length;
    uint8_t session_id[32]; // NOTE: we always reserve enough space for the SID
    uint8_t cipher_suite[2]; // negotiated cypher to use
    uint8_t compression_method;
    // uint16_t extensions_length; // consider leaving this out bc maybe it isn't being sent
};

template<typename T> uint8_t msg_type (); 
template<> uint8_t msg_type <ClientHello> ();
template<> uint8_t msg_type <ServerHello> ();

template<typename T>
struct Handshake {
    uint8_t msg_type = ::msg_type<T>(); // handshake type (client_hello) 
    uint8_t length1 = (sizeof(T) & 0xff0000) >> 16;             /* bytes in message */
    uint8_t length2 = (sizeof(T) & 0x00ff00) >> 8;
    uint8_t length3 = sizeof(T) & 0x0000ff; // <-- lsb
    T body;
};

template<typename T>
struct TLSPlaintext {
    uint8_t type = 22; // handshake
    uint8_t protocol_version[2] = {3,3}; // tls1.2 record
    uint16_t length = htons(sizeof(T));
    uint8_t fragment[sizeof(T)]; 
};
#pragma pack(pop)

TLSPlaintext<Handshake<ClientHello>> client_hello();
