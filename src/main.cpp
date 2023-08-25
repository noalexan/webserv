#include <iostream>
#include <unistd.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ExitCode.hpp>
#include <Config/Config.hpp>
#include <Client.hpp>

#define BUFFER_SIZE 256
#define MAX_EVENTS 10

void launch(Config const &config) {

	int kq = kqueue();

	if (kq == -1) {
		throw std::runtime_error("Error: kqueue() failed");
	}

	struct kevent events[MAX_EVENTS], changes;

	for (std::deque<Server>::const_iterator it = config.getServers().begin(); it != config.getServers().end(); it++) {
		EV_SET(&changes, it->fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
		if (kevent(kq, &changes, 1, nullptr, 0, nullptr) == -1) {
			throw std::runtime_error("Error: kevent() failed");
		}
	}

	std::map<int, Client> clients;

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

				if (events[i].flags & EV_EOF) {
					std::cout << "disconnect" << std::endl;

					uintptr_t const & fd = events[i].ident;
					EV_SET(&changes, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);

					if (kevent(kq, &changes, 1, NULL, 0, NULL) == -1) {
						throw std::runtime_error("kevent() failed");
					}
						
					if (close(events[i].ident) == -1) {
						throw std::runtime_error("close() failed");
					}

					clients.erase(events[i].ident);

					continue;
				}

				for (std::deque<Server>::const_iterator it = config.getServers().begin(); it != config.getServers().end(); it++) {
					if (events[i].ident == (uintptr_t) it->fd) {

						std::cout << "new connection" << std::endl;

						sockaddr_in client_address;
						socklen_t client_address_len = sizeof(client_address);
						int client_fd = accept(it->fd, (struct sockaddr *) &client_address, &client_address_len);
						fcntl(client_fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
						EV_SET(&changes, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr); // je t'ai vu jouer a la marelle

						timeout.tv_sec = 5; // sautillant sur le macadame
						timeout.tv_nsec = 0; // te voici maintenant jouvancelle

						if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) { // dans un corps pas tout a fait femme
							throw std::runtime_error("Error: kevent() failed"); // je vais t'apprendre un jeu extra
						} // qu'il ne faudra pas repeter

						clients[client_fd].server = &(*it); // la regle est simple tu verras
						clients[client_fd].request.setFd(client_fd); // il suffit juste de s'embrasser
						clients[client_fd].response.setFd(client_fd);

						break;
					}
				}

				if (clients.find(events[i].ident) != clients.end()) {
					switch (events[i].filter) {
						case EVFILT_READ:
							clients[events[i].ident].request.read();

							if (clients[events[i].ident].request.isFinished()) {
								EV_SET(&changes, events[i].ident, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);

								timeout.tv_sec = 5; // привет
								timeout.tv_nsec = 0;

								if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) {
									throw std::runtime_error("Error: kevent() failed");
								}

								clients[events[i].ident].request.parse(clients[events[i].ident].server);
								clients[events[i].ident].response.handle(clients[events[i].ident].request);
							}

							break;

						case EVFILT_WRITE:
							clients[events[i].ident].response.write();

							if (clients[events[i].ident].response.isFinished()) {

								if (close(events[i].ident) == -1) {
									throw std::runtime_error("close() failed");
								}

								clients.erase(events[i].ident);

								std::cout << "connection closed" << std::endl;
							}

							break;
					}
				}

			} catch (std::exception &e) {
				std::cerr << e.what() << std::endl;
				perror("");
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
	if (argc > 2) {
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
		config = Config((argc == 2) ? argv[1] : "webserv.conf");
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
// Version : 0.8