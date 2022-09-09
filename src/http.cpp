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

#include "numutils.h"
#include "SHA256.h"

#include <deque>

#include "rsa.h"
#include "cert.h"

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

bool verify_cert_chain(std::vector<X509v3> certs) {  
    for (int64_t i = certs.size() - 1; i >= 0; i--) {
        auto cert = certs.at(i);
        std::cout << "CERT-serial_number: " << cert.serial_number.as_decimal() << std::endl;
        // std::cout << "CERT-public-key: \n  modulus: " << cert.public_key.modulus.as_decimal() << "\n  exponent: "
        //    << cert.public_key.exponent.as_decimal() << std::endl;
        // std::cout << "CERT-signature: \n  " << cert.signature.data.as_decimal() << std::endl;

        PublicKey ca_pubkey;
        if (i == certs.size() - 1) {
        // RSA self sign check (only works on self signed). 
            ca_pubkey = cert.public_key;
        } else { // on others use the pubkey of the CA one above
            ca_pubkey = certs.at(i + 1).public_key;
        }
        std::vector<UnsignedBigInt> temp {cert.signature.data};
        auto decrypted_signature = decrypt(temp, ca_pubkey.exponent, ca_pubkey.modulus);
        // TODO: this is reversed somehow
        auto reversed_dec_sig = std::vector<uint8_t>(decrypted_signature.rbegin(),
                    decrypted_signature.rend());
        auto chopped_sig_hash = chop_decrypted_signature(reversed_dec_sig);
        auto const& hash = sha256_hash(cert.raw_bytes);

        std::vector<uint8_t> hash_as_octets;
        for (size_t k = 0; k < hash.size(); k++)
            for (int j = 3; j >= 0; j--)
                hash_as_octets.push_back((hash.at(k) >> (j * 8)) & 0xff);

        for (size_t p = 0; p < chopped_sig_hash.size(); p++)
            if (chopped_sig_hash[p] != hash_as_octets[p]) {
                std::cout << "HASH MISMATCH: decrypted: " << std::hex << std::setfill('0') 
                    << std::setw(2) << (int) chopped_sig_hash[p]
                    << " hashed cert: " << std::setw(2) << (int) hash_as_octets[p]
                    << std::dec << std::endl;
                return false;
            }
    }
    return true;
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
        
        auto length = (static_cast<uint16_t>(data[3]) << 8) | data[4]; // ntohs(*reinterpret_cast<uint16_t*>(&data[3]));
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
        int handshake_payload_l = btolEN24_u32((&data[1])); /* data[1] << 16 
            | data[2] << 8
            | data[3];
*/
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
                for (size_t i = 0; i < 2 + sizeof(Random) + 1; i++) {
                    ((uint8_t*) &decoded)[i] = data[0];
                    data.pop_front();
                }
                for (size_t i = 0; i < decoded.session_id_length; i++) {
                    decoded.session_id[i] = data[0];
                    data.pop_front();
                }
                for (size_t i = 0; i < 3; i++) {
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
                
                std::vector<X509v3> cert_chain;
                // NOTE: certificates.length not populated
                for (int i = 0; i < 3; i++) {
                    data.pop_front(); // NOTE: certificates.length not populated
                }

                for (size_t offset = 0; offset < handshake_payload_l - 3;) {
                    uint32_t cert_len = btolEN24_u32(data.begin()); 
                    for (size_t i = 0; i < 3; i++) {
                        data.pop_front();
                    }
                    
                    std::vector<uint8_t> cert_bytes;
                    for (size_t i = 0; i < cert_len; i++) {
                        cert_bytes.push_back(data[0]);
                        data.pop_front();
                    }
                    cert_chain.push_back(parse(cert_bytes));
                    offset += 3 + cert_len;
                }

                std::cout << "RESULT of certificate chain validation: " << std::to_string(
                        verify_cert_chain(std::move(cert_chain))) << std::endl;
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


