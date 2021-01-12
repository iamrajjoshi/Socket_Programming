#include "..\Server Multi Select\sockstuff.h"
#include <iostream>
#include <set>
#include <regex>
#include <stack>
#include <sstream>
#include <string>
#include <exception>


using namespace std;



#define DEBUG(x) cout << '>' << #x << ':' << x << endl
#define RESPONSE_SIZE 100000

struct link {
	string name;
	unsigned int depth;
};

stack <link> stk;
set <string> visited;

SOCKET makeSocket(char* hostname, char* path, char* port) {
	addrinfo hints, * bind_address;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(hostname, port, &hints, &bind_address);

	SOCKET server;
	if (bind_address == nullptr) {
		freeaddrinfo(bind_address);
		throw exception("null");
	}
		

	server = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
	connect(server, bind_address->ai_addr, bind_address->ai_addrlen);
	freeaddrinfo(bind_address);

	return server;
}

void sendRequest(SOCKET server, char* hostname, char* path, char* port) {
	stringstream header;
	header << "GET /" << path << " HTTP/1.1\r\n";
	header << "Host: " << hostname << ":" << port << "\r\n";
	header << "Connection: close\r\n";
	header << "User-Agent: honpwc web_get 1.0\r\n";
	header << "\r\n";

	send(server, header.str().c_str(), strlen(header.str().c_str()), 0);
	//cout << endl << endl << "Sent Header:\n" << header.str() << endl;
	return;
}

void parse_url(char* url, char* hostname, char* path) {
	char* s = strstr(url, "://");
	s += 3;
	char* e = strstr(s, "/");
	if (e == nullptr)
		e = strchr(s, '\0');
	char* h = hostname;
	for (char* p = s; p != e; ++p, ++h)
		*h = *p;
	++* h = '\0';
	char* a = path;
	if (*e != '\0') {
		for (char* p = ++e; *p != '\0'; ++p, ++a)
			*a = *p;
		++* a = '\0';
	}
	else {
		*path = '\0';
	}
	return;
}

void parse_url(string url, string& hostname, string& path) {
	int first = url .find("://");
	int last = url.find("/", first +4);
	hostname = url.substr(first+3, (last) - (first+3));
	path = url.substr(last + 1);
	//cout << hostname << endl << path << endl;
	return;
}


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


string fromWebsite(string input) {
	string hostname, path;
	//char* url = new char[4000];
	//strcpy(url, input.c_str());
	//char* hostname = new char[4000];
	//char* path = new char[4000];
	const char* port = "80";
	//parse_url(url, hostname, path);
	parse_url(input, hostname, path);
	string output;

	//cout << endl << "Hostname: " << endl << hostname << endl << "Path: " << endl << path << endl << endl;

	//SOCKET root = makeSocket(hostname, path, (char*)port);
	//sendRequest(root, hostname, path, (char*)port);
	SOCKET root;
	try {
		root = makeSocket((char*)hostname.c_str(), (char*)path.c_str(), (char*)port);
	}
	catch (exception e) {
		return "";
	}
	
	sendRequest(root, (char*)hostname.c_str(), (char*)path.c_str(), (char*)port);

	char* response = new char[RESPONSE_SIZE + 1];
	char* response_ptr = response;
	char* body = 0;
	int remaining = 0;

	const clock_t start_time = clock();
	while (true) {
		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(root, &reads);

		if ((clock() - start_time) / CLOCKS_PER_SEC > 4) {
			return "";
		}

		if (select(root + 1, &reads, 0, 0, 0) < 0) {
			fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
			exit(-1);
		}

		if (FD_ISSET(root, &reads)) {
			int bytes_received = recv(root, response_ptr, RESPONSE_SIZE, 0);

			if (bytes_received < 1) {
				//printf("\nConnection closed by peer.\n");
				break;
			}

			response_ptr += bytes_received;
			*response_ptr = '\0';//null terminate the end of the data recieved

			if (!body && (body = strstr(response, "\r\n\r\n"))) {
				*body = 0;//null terminate
				body += 4;//move off the \r\n\r\n

				//cout << endl << "Recieved Header: " << endl << response << endl;
				//cout << endl << "Received Body:" << endl;
			}
			if (body) {
				output.append(body);
			}
		}
	}

	//delete[] response;
	//delete[] hostname;
	//delete[] path;
	//delete[] url;
	return output;
}



int main(int argc, char* argv[]) {
	WinSockRAII w;
	if (argc < 3) {
		cout << "Root Depth\n" << endl;
		return 1;
	}
	int rd = atoi(argv[2]);
	
	regex exp(R"###(< *a *href *= *"http:[^"]*")###");//<a\s+href=["'].*?["']        // //http:\/\/.*")###"
	//i hate regex
	smatch sm;
	
	link l;
	l.depth = 1;
	l.name = argv[1];
	visited.insert(l.name);
	stk.push(l);

	while (!stk.empty()) {
		
		l = stk.top();
		stk.pop();

		for (int i = 0; i < l.depth-1; ++i)
			cout << "\t";
		cout << l.name << endl;

		string html = fromWebsite(l.name);

		//cout << html << endl;
		
		while (regex_search(html, sm, exp)) {
			
			string str = sm[0].str();
			
			unsigned first = str.find("\"");
			unsigned last = str.find_last_of("\"");
			str = str.substr(first+1, last -1 - first);
			//cout << str << endl;
			
			if (visited.find(str) == visited.end() && l.depth + 1 <= rd) {
				visited.insert(str);
				link nl;
				nl.depth = l.depth + 1;
				nl.name = str;
				stk.push(nl);
			}
			html = sm.suffix();
		}

	}

	return 0;
}