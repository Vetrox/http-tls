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

std::string request(const char* ip, std::string domain, char* payload, size_t payload_length) {
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
    std::string resp = "";
    while (true) {
        bzero(data, sizeof(data));
        int n = read(sck, data, sizeof(data));
        std::cout << "read " << n << " bytes" << std::endl;
        if (n == 0) { 
            std::cout << "INFO: Closing socket" << std::endl;
            close(sck);
            return resp;
        }
        if (n < 0) {
            std::cout << "ERROR " << n << ": Standard input error" << std::endl;
            abort();
        }

        for (int i = 0; i < n; i++) {
            resp += std::to_string(data[i]) + " ";
        }
    }
}
