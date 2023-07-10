#include <iostream>
#include <unistd.h>
#include "WebservConfig.hpp"

#define BUFFER_SIZE 1024

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

		std::cout << "\r\e[KWaiting for connection..." << std::flush;

		int clientSocket = accept(server.fd, (sockaddr *) &clientAddress, &clientAddressLength);

		if (clientSocket == -1) {
			std::cerr << "Failed to accept client connection." << std::endl;
			close(server.fd);
			exit(EXIT_FAILURE);
		}

		std::cout << "\r\e[KRequest received on port " << server.port << std::endl;

		std::cout << "\r\e[KReading request..." << std::flush;

		std::string request;

		while (request.find("\r\n\r\n") == std::string::npos) {

			char buffer[BUFFER_SIZE];

			ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

			if (bytesRead == -1) {
				std::cerr << "Failed to read request." << std::endl;
				close(clientSocket);
				exit(EXIT_FAILURE);
			}

			buffer[bytesRead] = '\0';

			request += buffer;

		}

		std::cout << "\r\e[KRequest read:\e[1;33m" << std::endl << request << "\e[0m";

		// TODO: Handle request

		std::cout << "\r\e[KSending response..." << std::flush;

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
// Version : 0.3.0
