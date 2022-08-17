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

#define IPADDR "93.184.216.34" // example.com

std::string request(const char*, std::string, std::string);

int main() {
    std::string resp = request(IPADDR, "example.com", "/");
    std::cout << resp << std::endl;
}

std::string request(const char* ip, std::string domain, std::string path) {
    int sck = 0;
    char data[1024];
    std::string send_payload = 
        "GET " + path + " HTTP/1.0\n"
        "Host: " + domain + "\n" "\n"; // has to end with double newline

    struct sockaddr_in ipOfServer;
    memset(data, '0' ,sizeof(data));
 
    if ((sck = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket not created \n");
        abort();
    }

    ipOfServer.sin_family = AF_INET;
    ipOfServer.sin_port = htons(80);
    ipOfServer.sin_addr.s_addr = inet_addr(ip);
 
    std::cout << "INFO: Connecting to " << ip << std::endl;
    if (connect(sck, (struct sockaddr*)& ipOfServer, sizeof(ipOfServer)) < 0) {
        printf("Connection failed due to port and ip problems\n");
        abort();
    }

    std::cout << "INFO: Sending request..." << std::endl;
    std::cout << send_payload << "%%%%%%"<< std::endl;
    write(sck, send_payload.c_str(), send_payload.length());

    std::cout << "INFO: Waiting for data..." << std::endl;
    std::string resp = "";
    while (true) {
        int n = read(sck, data, sizeof(data)-1);
        if (n == 0) { 
            close(sck);
            return resp;
        }
        if (n < 0) {
            std::cout << "ERROR: Standard input error" << std::endl;
            abort();
        }
        data[n] = 0; // ending \0 character
        resp += data;
    }
}
