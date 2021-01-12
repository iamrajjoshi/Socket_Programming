#include "..\Server Multi Select\sockstuff.h"
#include <iostream>
using namespace std;


#define DEBUG(x) cout << '>' << #x << ':' << x << endl

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

	if (argc != 2) {
		cout << "Correct Usage: Hostname\n";
		return -1;
	}

	//requirements to create the requirements to get possible sockets
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_ALL;

	addrinfo* server_addresses;

	getaddrinfo(argv[1], 0, &hints, &server_addresses);

	char address_buffer[100];
	struct addrinfo* address = server_addresses;
	
	if (address) {
		do {
			getnameinfo(address->ai_addr, address->ai_addrlen, address_buffer, sizeof(address_buffer), 0,0, NI_NUMERICHOST);
			cout << address_buffer << endl;
		} while ((address = address->ai_next));
	}

	freeaddrinfo(server_addresses);

	return 0;
}