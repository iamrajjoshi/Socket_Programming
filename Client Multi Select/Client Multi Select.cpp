#include "..\Server Multi Select\sockstuff.h"
#include <ctype.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
using namespace std;

struct WinSockRAII {
    WinSockRAII() {
        int iResult;
        WSADATA wsaData; //  a Winsock Struct with a lot of junk
        if (iResult = WSAStartup(MAKEWORD(2, 2), &wsaData)) {
            std::cout << "WSAStartup failed with error: " << iResult;
        }
    }
    ~WinSockRAII() {
        WSACleanup();   // clean up Winsock
    }
};

int main(int argc, char** argv) {
    WinSockRAII w;
    
    if (argc != 3) {
        cout << "Correct Usage: IP Adress Port#\n";
        return -1;
    }
    
    printf("Configuring local address...\n");
    
    //requirements to create the requirements to get possible sockets
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* server_address;

    getaddrinfo(argv[1], argv[2], &hints, &server_address);

    char address_buffer[100];
    char port_buffer[50];

    SOCKET server_socket;

    //take the first possibility and create a socket out of it
    server_socket = socket(server_address->ai_family, server_address->ai_socktype, server_address->ai_protocol);

    if (!ISVALIDSOCKET(server_socket)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return -1;
    }

    cout << "Connecting...\n";
    //connect server
    connect(server_socket, server_address->ai_addr, server_address->ai_addrlen);
	
    //free the linked list up
    freeaddrinfo(server_address);

    cout << "Server connected...\n";
    cout << "Enter a message...\n";
    

    while (true) {
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(server_socket, &reads);

        timeval t;
        memset(&t, 0, sizeof(t));
        t.tv_usec = 10'000;

        //hold here
        if (select(server_socket + 1, &reads, 0, 0, &t) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return -1;
        }

        if (FD_ISSET(server_socket, &reads)) {
            char read[4096];
            int bytes_received = recv(server_socket, read, 4096, 0);
            if (bytes_received < 1) {
                cout << "Server disconnected\n";
                break;
            }
            cout << "Recieved " << bytes_received << " Bytes: ";
            fwrite(read, 1, bytes_received, stdout);
        }

        if (_kbhit()) {
            char read[4096];
            // read in a line of user input text
            // send the text over the socket
            fgets(read, 4096, stdin);//make sure you don't read over 4096 bytes
            int bytes = send(server_socket, read, strlen(read), 0);
            cout << "Sent " << bytes << " bytes\n";

        }
    
    }

    printf("Closing listening socket...\n");
    CLOSESOCKET(server_socket);



    printf("Finished.\n");
    return 0;
}