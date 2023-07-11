#include "WebservConfig.hpp"
#include "Request.hpp"

#include <iostream>
#include <unistd.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

bool pathExists(std::string const & path) {
	return access(path.c_str(), R_OK) != -1;
}

bool isDirectory(std::string const & path) {
	struct stat st;
	if (stat(path.c_str(), &st) == 0) {
		return S_ISDIR(st.st_mode);
	}
	return false;
}

bool isFile(std::string const & path) {
	struct stat st;
	if (stat(path.c_str(), &st) == 0) {
		return S_ISREG(st.st_mode);
	}
	return false;
}

void listenPort(Server const & server) {

	if (bind(server.fd, (sockaddr*)&server.address, sizeof(server.address)) == -1) {
		std::cerr << "Failed to bind socket to address." << std::endl;
		close(server.fd);
		exit(EXIT_FAILURE);
	}

	if (listen(server.fd, 10) == -1) {
		std::cerr << "Failed to listen on socket." << std::endl;
		close(server.fd);
		exit(EXIT_FAILURE);
	}

	std::cout << "Listening on port " << server.port << "..." << std::endl;

}

void launchServer(Server const & server) {

	listenPort(server);

	std::cout << "Starting server on " << server.port << std::endl;

	while (true) {

		sockaddr_in clientAddress;
		socklen_t clientAddressLength = sizeof(clientAddress);

		std::cout << "\r\e[K\e[1;36mWaiting for connection...\e[0m" << std::flush;

		int clientSocket = accept(server.fd, (sockaddr *) &clientAddress, &clientAddressLength);

		if (clientSocket == -1) {
			std::cerr << "Failed to accept client connection." << std::endl;
			close(server.fd);
			continue;
		}

		std::cout << "\r\e[KRequest received on port " << server.port << std::endl;

		std::cout << "\r\e[K\e[1;36mReading request...\e[0m" << std::flush;

		std::string request;

		while (request.find("\r\n\r\n") == std::string::npos) {

			char buffer[BUFFER_SIZE];

			ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

			if (bytesRead == -1) {
				std::cerr << "Failed to read request." << std::endl;
				close(clientSocket);
				continue;
			}

			buffer[bytesRead] = '\0';

			request += buffer;

		}

		std::cout << "\r\e[KRequest read:\e[1;33m" << std::endl << request << "\e[0m";

		std::cout << "\r\e[K\e[1;36mParsing request...\e[0m" << std::flush;

		Request req(request);

		std::cout << "\r\e[KRequest parsed" << std::endl;

		std::cout << "\r\e[K\e[1;36mGetting root...\e[0m" << std::flush;

		std::string uri(req.uri());

		std::cout << "\r\e[K\e[1;35mchecking for location... (" << uri << ")\e[0m" << std::flush;

		while (server.locations.find(uri) == server.locations.end()) {

			if (uri.find('/') == std::string::npos) {
				std::cerr << "Error: no location found for " << req.uri() << std::endl;
				close(clientSocket);
				break;
			}

			uri = uri.substr(0, uri.find_last_of('/'));

			if (uri.empty()) uri = "/";

			std::cout << "\r\e[K\e[1;35mchecking for location... (" << uri << ")\e[0m" << std::flush;

		}

		if (server.locations.find(uri) == server.locations.end()) {
			continue;
		}

		Location const & location = server.locations.at(uri);
		uri = req.uri().substr(uri.length());

		std::cout << "\r\e[K\e[1;35mRoot: " << location.root << "\e[0m" << std::endl;

		std::cout << "\r\e[KSearching for file " << location.root + uri << "..." << std::flush;

		std::string filePath(location.root);

		if (filePath[filePath.length() - 1] != '/')
			filePath += '/';

		filePath += uri;

		if (!pathExists(filePath)) {

			std::cout << "\r\e[K\e[1;35m" << filePath << " doesn't exists\e[0m" << std::endl;

			std::cout << "\r\e[K\e[1;36mSending response...\e[0m" << std::flush;
			write(clientSocket, "HTTP/1.1 404 Not Found\r\n\r\n404 Not Found!\r\n", 42);
			std::cout << "\r\e[KResponse sent" << std::endl;
			close(clientSocket);
			continue;
		}

		if (isDirectory(filePath)) {

			if (filePath[filePath.length() - 1] != '/')
				filePath += '/';

			for (std::deque<std::string>::const_iterator it = location.indexes.begin(); it != location.indexes.end(); ++it) {

				if (isFile(filePath + *it)) {

					filePath += *it;
					break;

				}

			}

		}

		if (isDirectory(filePath)) {

			std::cout << "\r\e[K\e[1;35m" << filePath << " is a Directory\e[0m" << std::endl;

			std::cout << "\r\e[K\e[1;36mSending response...\e[0m" << std::flush;
			write(clientSocket, "HTTP/1.1 403 Forbidden\r\n\r\n403 Forbidden!\r\n", 42);
			std::cout << "\r\e[KResponse sent" << std::endl;
			close(clientSocket);
			continue;

		}

		std::cout << "\r\e[K\e[1;35mFile found: " << filePath << "\e[0m" << std::endl;

		std::cout << "\r\e[K\e[1;36mSending response...\e[0m" << std::flush;
		write(clientSocket, "HTTP/1.1 200 OK\r\n\r\nHello, World!\r\n", 34);
		std::cout << "\r\e[KResponse sent" << std::endl;

		close(clientSocket);

	}

}


int main(int argc, char** argv) {

	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config file>" << std::endl;
		exit(EXIT_FAILURE);
	}

	WebservConfig config(argv[1]);


	Server const & server = config.servers()[0];

	if (server.fd == -1) {
		std::cerr << "Failed to create socket." << std::endl;
		exit(EXIT_FAILURE);
	}

	launchServer(server);
	std::cout << "fin du programme" << std::endl;
	return (0);
}

// Authors : Charly Tardy, Marwan Ayoub, Noah Alexandre
// Version : 0.4.0
