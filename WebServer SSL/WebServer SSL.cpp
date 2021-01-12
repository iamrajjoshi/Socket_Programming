#define  _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/applink.c>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <cstring>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "C:\\Program Files\\OpenSSL-Win64\\lib\\VC\\libcrypto64MD.lib")
#pragma comment(lib, "C:\\Program Files\\OpenSSL-Win64\\lib\\VC\\libssl64MD.lib")

#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

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
int main() {
    WinSockRAII ws;

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        fprintf(stderr, "SSL_CTX_new() failed.\n");
        return 1;
    }

    const std::string cert_dir = "C:\\Users\\geeky\\Desktop\\";
    const std::string cert_pem = cert_dir + "cert.pem";
    const std::string key_pem = cert_dir + "key.pem";


    if (!SSL_CTX_use_certificate_file(ctx, cert_pem.c_str(), SSL_FILETYPE_PEM)
        || !SSL_CTX_use_PrivateKey_file(ctx, key_pem.c_str(), SSL_FILETYPE_PEM)) {
        fprintf(stderr, "SSL_CTX_use_certificate_file() failed.\n");
        ERR_print_errors_fp(stderr);
        return 1;
    }

    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* bind_address;
    getaddrinfo(0, "https", &hints, &bind_address);

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
        bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
        bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    while (1) {
        printf("Waiting for connection...\n");
        struct sockaddr_storage client_address;
        socklen_t client_len = sizeof(client_address);
        SOCKET socket_client = accept(socket_listen, (struct sockaddr*)&client_address, &client_len);


        if (!ISVALIDSOCKET(socket_client)) {
            fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        printf("Client is connected... ");
        char address_buffer[100];
        getnameinfo((struct sockaddr*)&client_address,
            client_len, address_buffer, sizeof(address_buffer), 0, 0,
            NI_NUMERICHOST);
        printf("%s\n", address_buffer);

        SSL* ssl = SSL_new(ctx);
        if (!ssl) {
            fprintf(stderr, "SSL_new() failed.\n");
            return 1;
        }

        SSL_set_fd(ssl, socket_client);
        if (SSL_accept(ssl) <= 0) {
            fprintf(stderr, "SSL_accept() failed.\n");
            ERR_print_errors_fp(stderr);

            SSL_shutdown(ssl);
            CLOSESOCKET(socket_client);
            SSL_free(ssl);

            continue;
        }

        printf("SSL connection using %s\n", SSL_get_cipher(ssl));

        printf("Reading request...\n");
        char request[1024];
        int bytes_received = SSL_read(ssl, request, 1024);
        printf("Received %d bytes.\n", bytes_received);
        //printf("%.*s", bytes_received, request);

        printf("Sending response...\n");
        const char* response =
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Local time is: ";
        int bytes_sent = SSL_write(ssl, response, strlen(response));
        printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));

        time_t timer;
        time(&timer);
        char* time_msg = ctime(&timer);
        bytes_sent = SSL_write(ssl, time_msg, strlen(time_msg));
        printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));

        printf("Closing connection...\n");
        SSL_shutdown(ssl);
        CLOSESOCKET(socket_client);
        SSL_free(ssl);
    }

    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);
    SSL_CTX_free(ctx);
    printf("Finished.\n");

    return 0;
}