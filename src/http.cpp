#include "http.h"
#include <iostream>

#include "sys/socket.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "string.h"
#include "arpa/inet.h"
#include <unistd.h>

#include <stdlib.h>
#include <errno.h>
#include "stdio.h"
#include <string>

#include <sstream>
#include <iomanip>

#include <deque>

//FIXME REMOVE THIS
#include "tls.h"

void try_decode(std::deque<uint8_t>&);

void request(const char* ip, std::string domain, uint8_t* payload, size_t payload_length) {
    int sck = 0;
    uint8_t data[1024];

    struct sockaddr_in ipOfServer; 
 
    if ((sck = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket not created \n");
        abort();
    }

    ipOfServer.sin_family = AF_INET;
    ipOfServer.sin_port = htons(443); // https
    ipOfServer.sin_addr.s_addr = inet_addr(ip);
 
    std::cout << "INFO: Connecting to " << ip << std::endl;
    if (connect(sck, (struct sockaddr*)& ipOfServer, sizeof(ipOfServer)) < 0) {
        printf("Connection failed due to port and ip problems\n");
        abort();
    }

    std::cout << "INFO: Sending request..." << std::endl;
    write(sck, payload, payload_length);

    std::cout << "INFO: Waiting for data..." << std::endl;
    std::deque<uint8_t> v_data;
    while (true) {
        bzero(data, sizeof(data));
        int n = read(sck, data, sizeof(data));
        if (n == 0) { 
            std::cout << "INFO: Closing socket" << std::endl;
            close(sck);
            return;
        }
        if (n < 0) {
            std::cout << "ERROR " << n << ": Standard input error" << std::endl;
            abort();
        }

        for (int i = 0; i < n; i++) {
            v_data.push_back(data[i]);
        }
        // std::cout << std::flush;


        try_decode(v_data);
    }
}

void try_decode(std::deque<uint8_t> &data) {
    if (data.size() < 5) {
        return;
    }
    std::cout << "try_decode" << std::endl;   
    uint16_t length = (data[3] << 8) | (data[4]); // use ntohs
    std::cout << "PCKLEN: " << length << std::endl;
    if (data.size() < length + 5) {
        return; // wait for tcp to finish
    }

    uint8_t record_payload [length]; // maybe stackoverflow
    memcpy(record_payload, &data[5], length);

    for (int i = 0; i < length + 5; i++) {
        data.pop_front();
    }

    auto sid_l = record_payload[
        1 + 3 + 2 + sizeof(Random)
    ];

    if (length != sizeof(Handshake<ServerHello>) - 32 + sid_l) {
        std::cout << "ASSERTION FAILED: Length was not Handshake<ServerHello>" << std::endl;
        abort();
    }

    ServerHello decoded;
    bzero(&decoded, sizeof(decoded));
    memcpy(&decoded, &record_payload[
        1 + 3
    ], 2+1);
    memcpy(&decoded.session_id, &record_payload[
        1 + 3 + 2 + sizeof(Random) + 1
    ], sid_l);
    memcpy(&decoded.cipher_suite, &record_payload[
        1 + 3 + 2 + sizeof(Random) + 1 + sid_l
    ], 3);

    std::cout << std::hex << std::setfill('0');
    std::cout << std::setw(2) 
        << "sv: " << +decoded.server_version[0] << " "
        << +decoded.server_version[1] << "\n"
        << "c: " << +decoded.compression_method << "\n"
        << "cs: " << +decoded.cipher_suite[0] << " " 
        << +decoded.cipher_suite[1] << "\n";
    for (int i = 0; i < sid_l; i++) {
        std::cout << +decoded.session_id[i] << " ";
    }
    std::cout << std::endl;
}

