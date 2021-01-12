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

#define DEBUG(x) std::cout << '>' << #x << ':' << x << std::endl

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


class TicTacToe {
public:
	char board[9]{ '0','1','2','3','4','5','6','7','8' };
	char player[2] = { 'X', 'O' };
	bool turn = false;
	void makeMove(int);
	int sendBoard(char*);
	int status();
};


int TicTacToe::status() {
	if (board[0] == board[1] && board[1] == board[2])
		return 1;

	else if (board[3] == board[4] && board[4] == board[5])
		return 1;

	else if (board[6] == board[7] && board[7] == board[8])
		return 1;

	else if (board[0] == board[3] && board[3] == board[6])
		return 1;

	else if (board[1] == board[4] && board[4] == board[7])
		return 1;

	else if (board[2] == board[5] && board[5] == board[8])
		return 1;

	else if (board[0] == board[4] && board[4] == board[8])
		return 1;

	else if (board[2] == board[4] && board[4] == board[6])
		return 1;

	else if (board[0] != '0' && board[1] != '1' && board[2] != '2' && board[3] != '3' && board[4] != '4' && board[5] != '5' && board[6] != '6' && board[7] != '7' && board[8] != '8')
		return 0;

	else
		return -1;
}

int TicTacToe::sendBoard(char* buf) {
	char* p = &buf[0];
	*p = '\n';
	p++;
	for (int i = 0; i < 9; ++i) {
		if ((i + 1) % 3 == 0) {
			*p = board[i];
			p++;
			*p = '\n';
			p++;

		}
		else {
			*p = board[i];
			p++;

		}

	}
	*p = '\n';
	return p - &buf[0] + 1;
}

void TicTacToe::makeMove(int i) {
	board[i] = player[turn];
	turn = !turn;
	return;
}





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




	TicTacToe game;
	int numPlayers = 0;
	SSL* players[2];
	char* playerIP[2];





	while (numPlayers < 2) {
		printf("Waiting for connection...\n");
		struct sockaddr_storage client_address;
		socklen_t client_len = sizeof(client_address);
		SOCKET socket_client = accept(socket_listen, (struct sockaddr*)&client_address, &client_len);
		std::cout << socket_client << std::endl;

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


		if (numPlayers == 0) {
			//strcpy(playerIP[numPlayers], address_buffer);
			players[numPlayers] = ssl;
			numPlayers++;
		}

		else if (numPlayers == 1) {
			//strcpy(playerIP[numPlayers], address_buffer);
			players[numPlayers] = ssl;
			numPlayers++;
		}

		std::cout << "Number of Players Ready: " << numPlayers << "\n";

	}
	char buf[100];
	int bytes = game.sendBoard(buf);
	SSL_write(players[0], buf, strlen(buf));
	SSL_write(players[1], buf, strlen(buf));
	

	while (game.status() == -1) {
		char request[1024];
		int bytes_received = SSL_read(players[game.turn], request, 1024);
		printf("Received %d bytes.\n", bytes_received);
		int pos = atoi(request);
		DEBUG(pos);
		game.makeMove(pos);
		if (game.status() != -1)
			break;
		char buf[100];
		int bytes = game.sendBoard(buf);
		SSL_write(players[game.turn], buf, strlen(buf));

	}
	if (game.status() == 1)
		std::cout << "Player " << game.player[!game.turn] << " wins!\n";//fix for server

	else
		std::cout << "Draw Game...\n";//fix for server


	SSL_shutdown(players[0]);
	SSL_shutdown(players[1]);

	SSL_free(players[0]);
	SSL_free(players[1]);

	printf("Closing listening socket...\n");
	CLOSESOCKET(socket_listen);
	SSL_CTX_free(ctx);
	printf("Finished.\n");

	return 0;
}