#include "..\Server Multi Select\sockstuff.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
#define DEBUG(x) cout << '>' << #x << ':' << x << endl
#define RESPONSE_SIZE 100000

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

int main(int argc, char* argv[]) {
	WinSockRAII w;

	if (argc < 2) {
		cout << "URL path\n" << endl;
		return 1;
	}

	char* hostname = argv[1];
	char* path = argv[2];
	const char* port = "80";

	addrinfo hints, * bind_address;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(hostname, port, &hints, &bind_address);

	SOCKET server;
	server = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
	connect(server, bind_address->ai_addr, bind_address->ai_addrlen);

	freeaddrinfo(bind_address);

	char header[2048];
	sprintf(header, "GET /%s HTTP/1.1\r\n", path);
	sprintf(header + strlen(header), "Host: %s:%s\r\n", hostname, port);
	sprintf(header + strlen(header), "Connection: close\r\n");
	sprintf(header + strlen(header), "User-Agent: honpwc web_get 1.0\r\n");
	sprintf(header + strlen(header), "\r\n");
	send(server, header, strlen(header), 0);
	cout << "Sent Header:\n" << header << endl;


	char* response = new char[RESPONSE_SIZE + 1];
	char* bounds = response, * ptr;
	char* end = response + RESPONSE_SIZE;
	char* body = 0;
	enum types { LENGTH, CHUNKED, NONE };
	types encoding = NONE;
	int remaining = 0;

	while (true) {

		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(server, &reads);

		if (select(server + 1, &reads, 0, 0, 0) < 0) {
			if (encoding == NONE)
				printf("%s", body);

			fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
			return 1;
		}

		if (FD_ISSET(server, &reads)) {
			int bytes_received = recv(server, bounds, RESPONSE_SIZE, 0);
			if (bytes_received < 1) {
				if (encoding == NONE && body) {
					printf("%.*s", (int)(end - body), body);
				}
				printf("\nConnection closed by peer.\n");
				break;
			}

			bounds += bytes_received;
			*bounds = '\0';//null terminate the end of the data recieved

			if (!body && (body = strstr(response, "\r\n\r\n"))) {
				*body = 0;//null terminate
				body += 4;//move off the \r\n\r\n

				cout << "Recieved Header: " << endl << response << endl;

				if (ptr = strstr(response, "Content-Length:")) {
					encoding = LENGTH;
					ptr = strchr(ptr, ' ');
					ptr += 1;
					remaining = strtol(ptr, 0, 10);
				}

				else if (ptr = strstr(response, "Transfer-Encoding: chunked")) {
					encoding = CHUNKED;
					remaining = 0;
				}

				cout << endl <<  "Received Body:" << endl;
			}
			if (body) {
				if (encoding == LENGTH) {
					if (bounds - body >= remaining) {
						printf("%.*s", remaining, body);
						break;
					}
				}
				else if (encoding == CHUNKED) {
					do {
						if (remaining == 0) {
							if ((ptr = strstr(body, "\r\n"))) {//if you found a chunk
								remaining = strtol(body, 0, 16);
								if (!remaining)//chunk is 0 --> done
									return 0;
								body = ptr + 2;
							}
							else {
								break;
							}
						}
						if (remaining && bounds - body >= remaining) {
							printf("%.*s", remaining, body);
							body += remaining + 2;
							remaining = 0;
						}
					} while (!remaining);
				}
			}
		}
	}
	delete[] response;
	return 0;
}