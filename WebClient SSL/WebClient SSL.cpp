#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
//#include <openssl/applink.c>

#include <cstring>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "C:\\Program Files\\OpenSSL-Win64\\lib\\VC\\libcrypto64MD.lib")
#pragma comment(lib, "C:\\Program Files\\OpenSSL-Win64\\lib\\VC\\libssl64MD.lib")


#define RESPONSE_SIZE 32768

#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())


#include <iostream>
#include <sstream>
#define TIMEOUT 5.0

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
int main_socket_code(int argc, char* argv[]);
constexpr int num_args = 0 /*2*/;
constexpr const char* arg_error = "Usage: Exe server port";

int main(int argc, char* argv[]) {
	WinSockRAII ws;
	if (argc == 4)
		return main_socket_code(argc, argv);
	else
		cout << arg_error;
	return 0;
}

void send_request(SSL* s, string& hostname, string& port, string& path) {
	stringstream ss;

	ss << "GET " << path << " HTTP/1.1\r\n";
	ss << "Host: " << hostname << ":" << port << "\r\n";
	ss << "Connection: close\r\n";
	ss << "User-Agent: someoldbrowser https_get 1.0\r\n";
	ss << "\r\n";

	SSL_write(s, ss.str().c_str(), ss.str().size());
	cout << "Sent Headers:\n" << ss.str();
}

SOCKET connect_to_host(const string& hostname, const string& port) {
	cout << "Configuring remote address...\n";
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	addrinfo* peer_address;
	if (getaddrinfo(hostname.c_str(), port.c_str(), &hints, &peer_address)) {
		cerr << "getaddrinfo() failed. " << GETSOCKETERRNO() << endl;
		exit(EXIT_FAILURE);
	}

	cout << "Remote address is: ";
	char address_buffer[100];
	char service_buffer[100];
	getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
		address_buffer, sizeof(address_buffer),
		service_buffer, sizeof(service_buffer),
		NI_NUMERICHOST);
	cout << address_buffer << '\t' << service_buffer << endl;

	cout << "Creating socket...\n";
	SOCKET server;
	server = socket(peer_address->ai_family,
		peer_address->ai_socktype, peer_address->ai_protocol);
	if (!ISVALIDSOCKET(server)) {
		cerr << "socket() failed. " << GETSOCKETERRNO() << endl;
		exit(EXIT_FAILURE);
	}

	cout << "Connecting...\n";
	if (connect(server,
		peer_address->ai_addr, peer_address->ai_addrlen)) {
		cerr << "connect() failed. " << GETSOCKETERRNO() << endl;
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(peer_address);

	cout << "Connected.\n\n";

	return server;
}

int main_socket_code(int argc, char* argv[]) {

	SSL_library_init();                         // Init library
	OpenSSL_add_all_algorithms();               // OpenSSL keeps an internal table of digest algorithms and ciphers. 
	SSL_load_error_strings();
	/*
	 * SSL_CTX_new() initializes the list of ciphers,
	 * the session cache setting, the callbacks,
	 * the keys and certificates and the options to its default values.
	 */
	SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());    // TLS_client_method() -  Negotiate highest available SSL/TLS version
	if (!ctx) {
		cerr << "SSL_CTX_new() failed.\n";
		return 1;
	}

	string hostname = argv[1];
	string port = argv[2];
	string path = argv[3];

	const std::string cert_dir = "C:\\Users\\geeky\\Desktop\\";
	const std::string cert_pem = cert_dir + "cert.pem";

	if (!SSL_CTX_load_verify_locations(ctx, cert_pem.c_str(), 0)) {
		cerr <<  "SSL_CTX_load_verify_locations() failed\n";
		return 1;
	}


	// This does nothing more than create the socket and connect to the server
	// like always
	SOCKET serv_socket = connect_to_host(hostname, port);

	/*
	 *SSL_new() creates a new SSL structure which is needed to hold the data for a TLS / SSL
	 *connection.  The new structure inherits the settings of the
	 *underlying context ctx :
	 *  connection method,
	 *  options,
	 *  verification settings,
	 *  timeout settings.
	 *
	 */
	SSL* ssl = SSL_new(ctx);
	if (!ssl) {
		cerr << "SSL_new() failed.\n";
		return 1;
	}

	if (!SSL_set_tlsext_host_name(ssl, hostname.c_str())) {   // set the hostname for SSL connection
		cerr << "SSL_set_tlsext_host_name() failed.\n";
		return 1;
	}

	SSL_set_fd(ssl, serv_socket);                               //  This sets the SSL connection to 'ride' on the socket connection

	if (SSL_connect(ssl) == -1) {                               // now make the SSL connection
		cerr << "SSL_connect() failed.\n";
		return 1;
	}
	long valid_cert = SSL_get_verify_result(ssl);
	if (valid_cert == X509_V_OK) {
		// Good
		cerr << "Certificates is Valid!!!\n";
	}
	else {
		// Bad
		cout << "Certificate NOT Verified " << valid_cert << endl;
		exit(1);
	}
	cout << "SSL/TLS using " << SSL_get_cipher(ssl) << "\n";          // we can see what cipher we are using


	X509* cert = SSL_get_peer_certificate(ssl);                 // returns a pointer to the X509 certificate
	if (!cert) {
		cerr << "SSL_get_peer_certificate() failed.\n";
		return 1;
	}

	char* tmp;
	if ((tmp = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0))) {  // get the subject of the cert - then make it human readable
		cout << "subject: " << tmp << "\n";
		OPENSSL_free(tmp);
	}

	if ((tmp = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0))) {  // get the issuer of the cert - then make it human readable
		cout << "issuer: " << tmp << "\n";
		OPENSSL_free(tmp);
	}

	X509_free(cert);                                                    // free up the cert

	send_request(ssl, hostname, port, path);                            // gen the HTTP request and sends it via SSL

	const clock_t start_time = clock();

	char response[RESPONSE_SIZE + 1];
	char* p = response, * q;
	char* end = response + RESPONSE_SIZE;
	char* body = 0;

	enum { length, chunked, connection };
	int encoding = 0;
	int remaining = 0;

	while (1) {

		if ((clock() - start_time) / CLOCKS_PER_SEC > TIMEOUT) {
			cerr << "timeout after %.2f seconds\n", TIMEOUT;
			return 1;
		}

		if (p == end) {
			cerr << "out of buffer space\n";
			return 1;
		}

		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(serv_socket, &reads);

		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 200;

		if (select(serv_socket + 1, &reads, 0, 0, &timeout) < 0) {
			cerr << "select() failed. (" << GETSOCKETERRNO() << ")\n";
			return 1;
		}

		if (FD_ISSET(serv_socket, &reads)) {
			int bytes_received = SSL_read(ssl, p, end - p);
			if (bytes_received < 1) {
				if (encoding == connection && body) {
					printf("%.*s", (int)(end - body), body);
				}

				printf("\nConnection closed by peer.\n");
				break;
			}


			p += bytes_received;
			*p = 0;

			if (!body && (body = strstr(response, "\r\n\r\n"))) {
				*body = 0;
				body += 4;

				cout << "Received Headers:\n" << response << '\n';

				q = strstr(response, "\nContent-Length: ");
				if (q) {
					encoding = length;
					q = strchr(q, ' ');
					q += 1;
					remaining = strtol(q, 0, 10);

				}
				else {
					q = strstr(response, "\nTransfer-Encoding: chunked");
					if (q) {
						encoding = chunked;
						remaining = 0;
					}
					else {
						encoding = connection;
					}
				}
				cout << "\nReceived Body:\n";
			}

			if (body) {
				if (encoding == length) {
					if (p - body >= remaining) {
						printf("%.*s", remaining, body);
						break;
					}
				}
				else if (encoding == chunked) {
					do {
						if (remaining == 0) {
							if ((q = strstr(body, "\r\n"))) {
								remaining = strtol(body, 0, 16);
								if (!remaining) goto finish;
								body = q + 2;
							}
							else {
								break;
							}
						}
						if (remaining && p - body >= remaining) {
							printf("%.*s", remaining, body);
							body += remaining + 2;
							remaining = 0;
						}
					} while (!remaining);
				}
			} //if (body)
		} //if FDSET
	} //end while(1)
finish:

	cout << "\nClosing socket...\n";
	SSL_shutdown(ssl);
	CLOSESOCKET(serv_socket);
	SSL_free(ssl);
	SSL_CTX_free(ctx);

	cout << "Finished.\n";
	return 0;
}