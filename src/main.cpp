#include <iostream>
#include <unistd.h>
#include <sys/event.h>
#include "WebservConfig.hpp"

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

// int signal_perso(int sigint, int close_ctrl) {
// 	if (sigint == 2) {
// 		std::cout << "changement de close_ctrl" << std::endl;
// 		close_ctrl = 1;
// 	}
// 	return close_ctrl;
// }

// void handle_signal(int signal, int server_fd) {
// 	if (signal == SIGINT) {
// 		std::cout << "Signal d'arrêt reçu. Arrêt du programme..." << std::endl;
// 		close(server_fd);
// 		exit(0);  // Terminaison du programme
// 	}
// }

void launchServer(Server const & server) {

	listenPort(server);

	// std::string	resp_header = "HTTP/1.1 200 OK\r\nServer: WebServ\r\nContent-Type: text/html\r\n\r\n";

	std::cout << "Starting server on " << server.port << std::endl;

	// kevent()

	while (true) {

		// signal(SIGINT, handle_signal);
		// signal(SIGINT, [&server](int signal) { handle_signal(signal, server.fd); });
		int close_ctrl = 0;
		// signal_perso(SIGINT, close_ctrl);




		sockaddr_in clientAddress;
		socklen_t clientAddressLength = sizeof(clientAddress);

		std::cout << "\r\e[KWaiting for connection..." << std::flush;

		int clientSocket = accept(server.fd, (sockaddr *) &clientAddress, &clientAddressLength);

		if (clientSocket == -1) {
			std::cerr << "Failed to accept client connection." << std::endl;
			close(server.fd);
			exit(EXIT_FAILURE);
		}

		std::cout << "\r\e[KReading request..." << std::flush;

		std::string request;
		char buffer[1024];
		while (read(clientSocket, buffer, 1024) == 1024) {
			request += buffer;
		}
		request += buffer;

		std::cout << "\r\e[KClient connected to port " << server.port << std::endl;

		write(clientSocket, "HTTP/1.1 200 OK\r\n\r\nHello, World!\r\n", 34);

		// if (request.find("GET / "))
		// {
		// 	std::string indexContent = resp_header + "\r\n" + "<html><head></head><body><h1>Hello World<h1></body></html>";
		// 	// std::cout << YELLOW << readIndexFile() << RESET << std::endl;
		// 	// std::cout << "to send : " << resp_header << "\r\n" << indexContent << std::endl;
		// 	write(clientSocket, indexContent.c_str(), indexContent.length());
		// }

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
