#include "http.h"
#include <iostream>

#include "certparser.h"

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

void request(const char* ip, uint8_t* payload, size_t payload_length) {
    int sck = 0;
    uint8_t data[4096];

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
    while (true) {
        if (data.size() < 5) {
            return;
        }
        if (data[0] != 22) {
            std::cout << "unsupported tls record" << std::endl;
            abort();
        }
        

        uint16_t length = ntohs(*(uint16_t*) &data[3]);
        if (data.size() < length + 5) {
            return; // wait for tcp to finish
        }
        std::cout << "try_decode: record_payload_length: " << length << std::endl;

        for (int i = 0; i < 5; i++) {
            data.pop_front();
        }

        if (length < 4) {
            std::cout << "Handshake type is not complete" << std::endl;
            abort();
        }

        constexpr auto handshake_preamble_l = 4;
        auto handshake_type = data[0];
        
        // FIXME: this depends on the host byte order (mine is len)
        auto handshake_payload_l = data[1] << 16 
            | data[2] << 8
            | data[3];

        if (length - handshake_preamble_l != handshake_payload_l) {
            std::cout << "Handshake payload_length discrepency"
                " to record_payload length" << std::endl;
            abort();
        }
        
        for (int i = 0; i < handshake_preamble_l; i++) {
            data.pop_front();
        }

        switch (handshake_type) {
            case msg_type<ServerHello>(): {
                std::cout << "Handshake Type: ServerHello" << std::endl;
                
                ServerHello decoded;
                bzero(&decoded, sizeof(decoded));
                for (int i = 0; i < 2 + sizeof(Random) + 1; i++) {
                    ((uint8_t*) &decoded)[i] = data[0];
                    data.pop_front();
                }
                for (int i = 0; i < decoded.session_id_length; i++) {
                    decoded.session_id[i] = data[0];
                    data.pop_front();
                }
                for (int i = 0; i < 3; i++) {
                    ((uint8_t*) &decoded.cipher_suite)[i] = data[0];
                    data.pop_front();
                }
                std::cout << std::hex << std::setfill('0');
                std::cout 
                    << "sv: " << std::setw(2) << (int) decoded.server_version[0] << " "
                    << std::setw(2) << (int) decoded.server_version[1] << "\n"
                    << "c: " << std::setw(2) << (int) decoded.compression_method << "\n"
                    << "cs: " << std::setw(2) << (int) decoded.cipher_suite[0] << " " 
                    << std::setw(2) << (int) decoded.cipher_suite[1] << "\n";
                for (int i = 0; i < decoded.session_id_length; i++) {
                    std::cout << std::setw(2) << (int) decoded.session_id[i] << " ";
                }
                std::cout << std::dec << std::endl;
                break;
            }
            case msg_type<Certificates>(): {
                std::cout << "it's certificates" << std::endl;
                
                Certificates certificates;
                // NOTE: certificates.length not populated
                for (int i = 0; i < 3; i++) {
                    data.pop_front(); // NOTE: certificates.length not populated
                }

                for (int offset = 0; offset < handshake_payload_l - 3;) {
                    int cert_len = btolEN24_u32((&data[0])); 
                    for (int i = 0; i < 3; i++) {
                        data.pop_front();
                    }
                    std::cout << "certlen: " << cert_len << std::endl;
                    
                    Certificate cert;
                     std::cout << std::hex << std::setfill('0');
                    for (int i = 0; i < cert_len; i++) {
                        cert.bytes.push_back(data[0]);
                         std::cout << std::setw(2) <<  (int) data[0];
                        data.pop_front();
                    }
                    std::cout << std::dec << std::endl;
                    parse(cert.bytes);
                    certificates.certs.push_back(cert);
                    offset += 3 + cert_len;
                    std::cout << std::endl;
                }

                std::cout << "NumofCerts: " << +certificates.certs.size() << std::endl;
                

                break;
            }
            case msg_type<ServerHelloDone>(): {
                std::cout << "it's serverhellodone" << std::endl;
                // TODO: we abort here so we quit faster.
                abort();
                break;
            }
            default:
                std::cout << "Unsupported Handshake type: "
                    << + handshake_type << std::endl;
                abort();
        }
    }
}

