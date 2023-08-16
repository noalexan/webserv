#include <iostream>
#include <unistd.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ExitCode.hpp>
#include <Config/Config.hpp>
#include <Request/Request.hpp>
#include <Response/Response.hpp>

#define BUFFER_SIZE 256
#define MAX_EVENTS 10

void launch(Config const &config) {

	int kq = kqueue();

	if (kq == -1) {
		throw std::runtime_error("Error: kqueue() failed");
	}

	struct kevent events[MAX_EVENTS], changes;

	for (std::deque<Server>::const_iterator it = config.getServers().begin(); it != config.getServers().end(); it++) {
		EV_SET(&changes, it->fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
		if (kevent(kq, &changes, 1, nullptr, 0, nullptr) == -1) {
			throw std::runtime_error("Error: kevent() failed");
		}
	}

	std::map<int, Server const *> clients;

	while (true) {

		timespec timeout;
		timeout.tv_sec = 5;
		timeout.tv_nsec = 0;

		int nev = kevent(kq, nullptr, 0, events, MAX_EVENTS, &timeout);

		if (nev == -1) {
			std::cerr << "Error: kevent() failed" << std::endl;
			continue;
		}

		for (int i = 0; i < nev; i++) {

			try {

				Server const * server = nullptr;

				for (std::deque<Server>::const_iterator it = config.getServers().begin(); it != config.getServers().end(); it++) {
					if (events[i].ident == (uintptr_t) it->fd) {

						sockaddr_in client_address;
						socklen_t client_address_len = sizeof(client_address);
						int client_fd = accept(it->fd, (struct sockaddr *) &client_address, &client_address_len);
						fcntl(client_fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
						EV_SET(&changes, client_fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);

						timeout.tv_sec = 5;
						timeout.tv_nsec = 0;

						if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) {
							throw std::runtime_error("Error: kevent() failed");
							continue;
						}

						server = &(*it);
						clients[client_fd] = &(*it);

						break;

					}
				}

				if (server == nullptr && events[i].filter == EVFILT_READ) {

					std::string _;
					char buffer[BUFFER_SIZE + 1];
					ssize_t bytes_read;

					do {
						
						bytes_read = read(events[i].ident, buffer, BUFFER_SIZE);

						if (bytes_read == -1) {
							throw std::runtime_error("Error: read() failed");
						}

						buffer[bytes_read] = '\0';
						_.append(buffer, bytes_read);

					} while (bytes_read == BUFFER_SIZE);

					std::cout << "\e[33;1m" << _ << "\e[0m" << std::endl;

					try {
						Request request(_, clients[events[i].ident]);
						Response response(request, events[i].ident, config);
					} catch(std::exception & e) {
						std::cerr << e.what() << std::endl;
						write(events[i].ident, "HTTP/1.1 500 Internal Server Error\r\n", 36);
						write(events[i].ident, "Content-Type: text/plain\r\n\r\n", 28);
						write(events[i].ident, "Internal Server Error\r\n", 23);
					}

					clients.erase(events[i].ident);

					if (close(events[i].ident) == -1) {
						std::cerr << "Error: close() failed" << std::endl;
					}

				}

			} catch (std::exception &e) {
				std::cerr << e.what() << std::endl;
			}

		}

	}

}

void listen(Server const &server) {

	int opt = 1;
	if (setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		throw std::runtime_error("Error: setsockopt() failed");
	}

	if (bind(server.fd, (struct sockaddr *) &server.address, sizeof(server.address)) == -1) {
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

int main(int argc, char ** argv) {

	// Check arguments
	if (argc != 2) {
		std::cerr << "Usage: " << ((argc) ? argv[0] : "webserv") << " <config_file>" << std::endl;
		return USAGE_FAILURE;
	}

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		std::cerr << "Error: signal() failed" << std::endl;
		return SIGNAL_FAILURE;
	}

	// Getting config
	Config config;
	try {
		config = Config(argv[1]);
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
