#include <iostream>
#include <unistd.h>
#include <sys/event.h>
#include <ExitCode.hpp>
#include <WebservConfig/WebservConfig.hpp>

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

void launch(WebservConfig const &config) {

	int kq = kqueue();
	if (kq == -1) {
		throw std::runtime_error("Error: kqueue() failed");
	}

	struct kevent events[MAX_EVENTS];

	for (std::deque<Server>::const_iterator it = config.servers().begin(); it != config.servers().end(); it++) {
		EV_SET(&events[it->fd], it->fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
		if (kevent(kq, &events[it->fd], 1, NULL, 0, NULL) == -1) {
			if (close(kq) == -1) {
				std::cerr << "Error: close() failed" << std::endl;
			}
			throw std::runtime_error("Error: kevent() failed");
		}
	}

	while (true) {

		int nevents = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
		if (nevents == -1) {
			if (close(kq) == -1) {
				std::cerr << "Error: close() failed" << std::endl;
			}
			throw std::runtime_error("Error: kevent() failed");
		}

		for (int i = 0; i < nevents; i++) {

			int fd = events[i].ident;

			Server const *server = nullptr;
			for (std::deque<Server>::const_iterator it = config.servers().begin(); it != config.servers().end(); it++) {
				if (it->fd == fd) {
					server = &(*it);
					break;
				}
			}

			if (server == nullptr) {
				std::cerr << "Error: server not found" << std::endl;
				continue;
			}

			if (events[i].flags & EV_EOF) {
				std::cout << "Client disconnected" << std::endl;
				continue;
			}

			if (events[i].flags & EV_ERROR) {
				std::cerr << "Error: EV_ERROR" << std::endl;
				continue;
			}

			int client_fd = accept(server->fd, nullptr, nullptr);
			if (client_fd == -1) {
				std::cerr << "Error: accept() failed" << std::endl;
				continue;
			}

			char buffer[BUFFER_SIZE];
			ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE);
			if (bytes_read == -1) {
				std::cerr << "Error: read() failed" << std::endl;
				if (close(client_fd) == -1) {
					std::cerr << "Error: close() failed" << std::endl;
				}
				continue;
			}

			std::cout << "Received " << bytes_read << " bytes" << std::endl;
			std::cout << buffer << std::endl;

			std::string response =	"HTTP/1.1 200 OK\r\n"
									"Content-Type: text/plain\r\n"
									"Content-Length: 12\r\n"
									"\r\n"
									"Hello world!";

			ssize_t bytes_written = write(client_fd, response.c_str(), response.length());
			if (bytes_written == -1) {
				std::cerr << "Error: write() failed" << std::endl;
				if (close(client_fd) == -1) {
					std::cerr << "Error: close() failed" << std::endl;
				}
				continue;
			}

			std::cout << "Sent " << bytes_written << " bytes" << std::endl;

			if (close(client_fd) == -1) {
				std::cerr << "Error: close() failed" << std::endl;
			}

		}

	}

}

void listen(Server const &server) {
	
	int opt = 1;
	if (setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		throw std::runtime_error("Error: setsockopt() failed");
	}

	if (bind(server.fd, (struct sockaddr *)&server.address, sizeof(server.address)) == -1) {
		throw std::runtime_error("Error: bind() failed");
	}

	if (listen(server.fd, 10) == -1) {
		throw std::runtime_error("Error: listen() failed");
	}

	std::cout << "Listening on port " << server.port << "..." << std::endl;

}

void cleanup(WebservConfig const &config) {
	std::cout << "Cleaning up..." << std::endl;
	for (std::deque<Server>::const_iterator it = config.servers().begin(); it != config.servers().end(); it++) {
		std::cout << "Closing port " << it->port << "..." << std::endl;
		if (close(it->fd) == -1) {
			throw std::runtime_error("Error: close() failed");
		}
	}
}

int main(int argc, char **argv) {

	// Check arguments
	if (argc != 2) {
		std::cerr << "Usage: " << ((argc) ? argv[0] : "webserv") << " <config_file>" << std::endl;
		return USAGE_FAILURE;
	}

	// Getting config
	WebservConfig config;
	try {
		config = WebservConfig(argv[1]);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		try {
			cleanup(config);
		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
			return CLEANUP_FAILURE;
		}
		return CONFIG_FAILURE;
	}

	// Listening
	try {
		for (std::deque<Server>::const_iterator it = config.servers().begin(); it != config.servers().end(); it++) {
			listen(*it);
		}
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		try {
			cleanup(config);
		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
			return CLEANUP_FAILURE;
		}
		return LISTEN_FAILURE;
	}

	// Launching
	try {
		launch(config);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		try {
			cleanup(config);
		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
			return CLEANUP_FAILURE;
		}
		return LAUNCH_FAILURE;
	}

	// Cleanup
	try {
		cleanup(config);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return CLEANUP_FAILURE;
	}

	return SUCCESS;

}
