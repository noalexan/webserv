#include <iostream>
#include <unistd.h>
#include "WebservConfig.hpp"

void listenPort(Server const & server) {

	if (bind(server.fd, (sockaddr*)&server.address, sizeof(server.address)) == -1) {
        std::cerr << "Failed to bind socket to address." << std::endl;
        close(server.fd);
        exit(1);
    }

    // Listen for incoming connections
    if (listen(server.fd, 10) == -1) {
        std::cerr << "Failed to listen on socket." << std::endl;
        close(server.fd);
        exit(1);
    }

    std::cout << "Listening on port " << server.port << "..." << std::endl;

}

void launchServer(Server const & server) {

	listenPort(server);

	std::cout << "Starting server on " << server.port << std::endl;

	sockaddr_in clientAddress;
	socklen_t clientAddressLength = sizeof(clientAddress);

	while (true) {

		std::cout << "Waiting for connection..." << std::flush;

		int clientSocket = accept(server.fd, (sockaddr *) &clientAddress, &clientAddressLength);

		if (clientSocket == -1) {
			std::cerr << "Failed to accept client connection." << std::endl;
			close(server.fd);
			exit(1);
		}

		std::cout << "\r\e[KClient connected." << std::endl;

		write(clientSocket, "HTTP/1.1 200 OK\r\n\r\nHello, World!\r\n", 34);

		close(clientSocket);
	}

}

int main(int argc, char** argv) {

	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config file>" << std::endl;
		return (1);
	}

	WebservConfig config(argv[1]);

	Server const & server = config.servers()[0];

	if (server.fd == -1) {
		std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

	launchServer(server);

	return (0);
}

// Authors : Charly Tardy, Marwan Ayoub, Noah Alexandre
// Version : 0.3.0
