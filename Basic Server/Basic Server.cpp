
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <ctime>
using namespace std;

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment(lib, "iphlpapi.lib")

#define DEFAULT_BUFLEN 51200
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())




int ServerBasicIPv4SansErrorChecking() {
    /*
     * Overview of a basic server
     *  Create a Socket
     *  Bind a listening socket to the network card
     *  Listen on a socket
     *  Accept connection
     *  Receive data
     *  Send an answer
     *  Disconnect  client socket
     *  Disconnect  listening socket when done for good
     *
     * Below is the details
     *  memset hints struct to 0 (important some fields MUST be 0)
     *  set hints family, socktype and flags
     *  getaddrinfo()    pass hints,port/service, get addrinfo (used in socket and bind below)
     *  socket() -       pass addrinfo family socktype,protocol -> listening socket
     *  bind()           pass listening socket, addrinfo  addr and addrlen
     *  freeaddrinfo()   pass addrinfo (done with it)
     *  listen()         pass listening socket and queue length
     *  accept()         pass listening socket, ptr to sockaddr for client address and ptr int for sockaddr len
     *                   -> socket to the client!  (accept() blocks until there is a connection)
     *  getnameinfo()opt gets client address info (IP address for example)
     *  recv()           pass socket to client from accept(), request string from client, len of request , opt flags(set 0 norm)
     *  send()           pass socket to client, response from server, response length, flags (linux and win are VERY diff on flags)
     *  closesocket      pass socket to client.  Done transmission - NOTE the listening socket is STILL valid and can be used in accept() again
     *  (linux close())
     *  closesocket      pass listening socket - ONLY do this when your service no longer wants to accept connections.
     *
     */


    cout << "Configuring local address...\n";
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));       // ai_addrlen, ai_canonname, ai_addr, and ai_next members MUST be zero 
    hints.ai_family = AF_INET;              // IPv4 TCP or UDP  
    hints.ai_socktype = SOCK_STREAM;        // TCP ( SOCK_DGRAM for UDP)
    hints.ai_flags = AI_PASSIVE;            // The address will be used in a call to bind()

    addrinfo* bind_address;
    /* getaddrinfo function is used to determine the values in the sockaddr structure:
     * The hints structure gives it information that helps it decide
     * what to return in the addrinfo structure.
       If the AI_PASSIVE flag is specified in hints.ai_flags, and node is
       NULL, then the returned socket addresses will be suitable for
       bind(2)ing a socket that will accept(2) connections.  The returned
       socket address will contain the "wildcard address" (INADDR_ANY for
       IPv4 addresses, IN6ADDR_ANY_INIT for IPv6 address).  The wildcard
       address is used by applications (typically servers) that intend to
       accept connections on any of the host's network addresses.
    */
    getaddrinfo(NULL,                     // node should be null and hints.ai_flags= AI_PASSIVE to be a socket server  (otherwise the host we want to connect to)
                "8080",                   // the port OR service name (listed in %WINDIR%\system32\drivers\etc\services)
                &hints,                   // the hints set above... 
                &bind_address);           /* ptr to ptr -
                                                a linked list of addrinfo structures, one for each network address that matches node
                                                and service, subject to any restrictions imposed by hints, and
                                                returns a pointer to the start of the list in res.  The items in the
                                                linked list are linked by the ai_next field.
                                                There are several reasons why the linked list may have more than one
                                                addrinfo structure, including: the network host is multihomed, acces-
                                                sible over multiple protocols (e.g., both AF_INET and AF_INET6); or
                                                the same service is available from multiple socket types (one
                                                SOCK_STREAM address and another SOCK_DGRAM address, for example).
                                                Normally, the application should try using the addresses in the order
                                                in which they are returned.  The sorting function used within getad-
                                                drinfo() is defined in RFC 3484; the order can be tweaked for a par-
                                                ticular system by editing /etc/gai.conf (available since glibc 2.5).
                                          */

    cout << "Creating socket...\n";
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,    // the Family type (AF_INET in this case)
                           bind_address->ai_socktype,  // the socket type (should be SOCK_STREAM) 
                           bind_address->ai_protocol); // the protocol (in this case it will be IPPROTO_IP but could be IPPROTO_IPV4/IPPROTO_IPV6)

    auto err = bind(socket_listen,                     // listening socket
                    bind_address->ai_addr,             // the address we are bound to
                    bind_address->ai_addrlen);         // len of address 

    freeaddrinfo(bind_address);                        // frees up the linked list...


    cout << "Listening...\n";
    err = listen(socket_listen,                         // we listen on the socket that came back from socket.
                 10);                                   // we queue up to 10 connections before rejection 


    cout <<"Waiting for connection...\n";

    SOCKET socket_client;
    sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    // accept() blocks until a request comes in (note:  Not used with UDP/Datagrams) 
    socket_client = accept(socket_listen,               // return is socket_client, is NEW socket we talk to client on
                           (sockaddr*)&client_address,  // sockaddr struct contains address of client
                           &client_len);                // len of client address

    cout << "Client is connected... ";
    char address_buffer[100];

    //inverse of getaddrinfo()-converts a socket address to a corresponding host and service
    getnameinfo((sockaddr*)&client_address,             // sockaddr of client address
        client_len,                                     // len of client address
        address_buffer,                                 // string representation of client address
        sizeof(address_buffer),                         // size of string
        0,                                              // string rep of server address
        0,                                              // len of server address
        NI_NUMERICHOST);                                // flags - NI_NUMERICHOST means give IP address
    cout << address_buffer << endl;


    cout << "Reading request...\n";
    char request[1025];
    memset(request, 0, 1025);
    int received;
                                                        
    received = recv(socket_client,                      // socket to client
                    request,                            // request buffer
                    1024,                               // request buffer size 
                    0);                                 // flags - a neat one is MSG_PEEK which peeks at the message
                                                        //         but it is not removed from Queue
    cout << received << "\t" << request << endl;;


    cout << "Sending response...\n";
    const string response =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "Local time is: ";
    int bytes_sent;

    bytes_sent = send(socket_client,                    // socket to the client                    
                      response.c_str(),                 // Server's response
                      response.size(),                  // len of response
                      0);                               // flags (not much for winsock)

    time_t timer;
    time(&timer);
    char* time_msg = ctime(&timer);
    bytes_sent = send(socket_client, 
                      time_msg, 
                      strlen(time_msg), 
                      0);

    CLOSESOCKET(socket_client);

    CLOSESOCKET(socket_listen);

    cout <<"Finished.\n";

    return 0;
}

// Inits and closes Winsock library 
struct WinSockRAII {
    int iResult;
    WinSockRAII() {
        WSADATA wsaData; //  a Winsock Struct with a lot of junk
        if (iResult = WSAStartup(MAKEWORD(2, 2), &wsaData)) {
            cout << "WSAStartup failed with error: " << iResult;
        }
    }
    ~WinSockRAII() {
        WSACleanup();   // clean up Winsock
    }
};

int  main(int argc, char** argv) {
    WinSockRAII ws;
    if (ws.iResult) return -1;

    ServerBasicIPv4SansErrorChecking();
    return 0;
}