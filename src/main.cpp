#include <iostream>
#include <unistd.h>
#include <sys/event.h>
#include <ExitCode.hpp>
#include <Config/Config.hpp>
#include <Request/Request.hpp>
#include <Response/Response.hpp>

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

void launch(Config const &config) {

	int kq = kqueue();
	if (kq == -1) {
		throw std::runtime_error("Error: kqueue() failed");
	}

	struct kevent events[MAX_EVENTS];

	for (std::deque<Server>::const_iterator it = config.getServers().begin(); it != config.getServers().end(); it++) {
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

			std::cout << "new connection" << std::endl;

			int fd = events[i].ident;

			Server const *server = nullptr;
			for (std::deque<Server>::const_iterator it = config.getServers().begin(); it != config.getServers().end(); it++) {
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

			std::cout << "Accepted connection" << std::endl;
			std::cout << "reading request..." << std::endl;

			std::string _;
			char buffer[BUFFER_SIZE];
			ssize_t bytes_read;

			while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE))) {

				if (bytes_read == -1) {
					std::cerr << "Error: read() failed" << std::endl;
					write(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n", 38);
					if (close(client_fd) == -1) {
						std::cerr << "Error: close() failed" << std::endl;
					}
					continue;
				}

				if (bytes_read == 0) {
					std::cout << "Client disconnected" << std::endl;
					write(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n", 38);
					if (close(client_fd) == -1) {
						std::cerr << "Error: close() failed" << std::endl;
					}
					continue;
				}

				buffer[bytes_read] = '\0';

				_ += buffer;

				if (bytes_read < BUFFER_SIZE) {
					break;
				}

			}

			try {
				Request		request(_, server);
				Response	response(request, client_fd, config);
			} catch (std::exception const &e) {
				std::cerr << e.what() << std::endl;
				write(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n", 38);
			}

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

void cleanup(Config const &config) {
	std::cout << "Cleaning up..." << std::endl;
	for (std::deque<Server>::const_iterator it = config.getServers().begin(); it != config.getServers().end(); it++) {
		std::cout << "Closing port " << it->port << "..." << std::endl;
		if (close(it->fd) == -1) {
			throw std::runtime_error("Error: close() failed");
		}
	}
}

int main(int argc, char ** argv, char ** env) {

	// Check arguments
	if (argc != 2) {
		std::cerr << "Usage: " << ((argc) ? argv[0] : "webserv") << " <config_file>" << std::endl;
		return USAGE_FAILURE;
	}

	// Getting config
	Config config;
	try {
		config = Config(argv[1], env);
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
		for (std::deque<Server>::const_iterator it = config.getServers().begin(); it != config.getServers().end(); it++) {
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

// Authors : Charly Tardy, Marwan Ayoub, Noah Alexandre
// Version : 0.7.1
