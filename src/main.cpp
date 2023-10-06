#include <iostream>
#include <unistd.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <utils/ExitCode.hpp>
#include <utils/Colors.hpp>
#include <Config/Config.hpp>
#include <Client.hpp>

#define MAX_EVENTS 512
#define TIMEOUT_S 2

/*

	TODO:
		- Make the CGI request works with POST method too

*/

void launch(Config const &config) {

	int kq = kqueue();

	if (kq == -1) {
		throw std::runtime_error("kqueue() failed");
	}

	struct kevent events[MAX_EVENTS], changes;

	std::vector<Server> const & servers = config.getServers();
	std::map<int, Client> clients;

	for (std::vector<Server>::const_iterator server = servers.begin(); server != servers.end(); server++) {
		for (std::vector<Address>::const_iterator address = server->addresses.begin(); address != server->addresses.end(); address++) {
			EV_SET(&changes, address->fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
			if (kevent(kq, &changes, 1, nullptr, 0, nullptr) == -1) {
				throw std::runtime_error("kevent() failed");
			}
		}
	}

	timespec timeout;
	int nev;
	while (true) {

		timeout.tv_sec = 5;
		timeout.tv_nsec = 0;

		if ((nev = kevent(kq, nullptr, 0, events, MAX_EVENTS, &timeout)) == -1) {
			std::cerr << "kevent() failed" << std::endl;
			continue;
		}

		for (int i = 0; i < nev; i++) {

			try {

				if (events[i].flags & EV_EOF) {

					timeout.tv_sec = 5;
					timeout.tv_nsec = 0;

					EV_SET(&changes, events[i].ident, events[i].filter, EV_DELETE, 0, 0, nullptr);
					if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) {
						throw std::runtime_error("kevent() failed");
					}

					if (close(events[i].ident) == -1) {
						throw std::runtime_error("close() failed");
					}

					clients.erase(events[i].ident);

					std::cout << BMAG << "disconnect" << CRESET << std::endl;

				}
				
				bool isServer = false;
				for (std::vector<Server>::const_iterator server = servers.begin(); server != servers.end() and not isServer; server++) {
					for (std::vector<Address>::const_iterator address = server->addresses.begin(); address != server->addresses.end() and not isServer; address++) {
						if (address->fd == (int) events[i].ident) {

							std::cout << "receiving request from server '" << server->servername << '\'' << std::endl;
							isServer = true;

							sockaddr_in client_address;
							socklen_t client_address_len = sizeof(sockaddr_in);
							int client_fd = accept(events[i].ident, (sockaddr *) &client_address, &client_address_len);

							if (client_fd == -1) {
								throw std::runtime_error("accept() failed");
							}

							fcntl(client_fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
							EV_SET(&changes, client_fd, EVFILT_READ, EV_ADD, 0, 0, nullptr); // je t'ai vu jouer a la marelle

							timeout.tv_sec = 5; // sautillant sur le macadame
							timeout.tv_nsec = 0; // te voici maintenant jouvancelle

							if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) { // dans un corps pas tout a fait femme
								throw std::runtime_error("kevent() failed"); // je vais t'apprendre un jeu extra
							} // qu'il ne faudra pas repeter

							timeout.tv_sec = 5; // la regle est simple tu verras
							timeout.tv_nsec = 0; // il suffit juste de s'embrasser

							EV_SET(&changes, client_fd, EVFILT_TIMER, EV_ADD, NOTE_SECONDS, TIMEOUT_S, nullptr);
							if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) {
								throw std::runtime_error("kevent() failed");
							}

							Client client;

							client.server = &*server;
							client.request.setFd(client_fd);
							client.response.setFd(client_fd);

							clients[client_fd] = client;

							std::cout << BGRN << "new client" << CRESET << std::endl;
						}
					}
				}

				if (not isServer and clients.find(events[i].ident) != clients.end()) {
					switch (events[i].filter) {

						case EVFILT_READ:

							clients[events[i].ident].request.read(clients[events[i].ident].server->max_client_body_size);

							if (clients[events[i].ident].request.isFinished()) {

								timeout.tv_sec = 5;
								timeout.tv_nsec = 0;

								EV_SET(&changes, events[i].ident, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
								if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

								timeout.tv_sec = 5;
								timeout.tv_nsec = 0;

								EV_SET(&changes, events[i].ident, EVFILT_TIMER, EV_DELETE, 0, 0, nullptr);
								if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

								timeout.tv_sec = 5;
								timeout.tv_nsec = 0;

								EV_SET(&changes, events[i].ident, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
								if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

								if (clients[events[i].ident].request.isTooLarge()) {
									clients[events[i].ident].response.payloadTooLarge(clients[events[i].ident].server);
								} else {
									clients[events[i].ident].request.parse(clients[events[i].ident].server);
									clients[events[i].ident].response.handle(clients[events[i].ident].request, clients[events[i].ident].server, config, false);
								}

							}

							break;

						case EVFILT_WRITE:

							clients[events[i].ident].response.write();

							if (clients[events[i].ident].response.isFinished()) {

								timeout.tv_sec = 5;
								timeout.tv_nsec = 0;

								EV_SET(&changes, events[i].ident, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
								if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");
								if (close(events[i].ident) == -1) throw std::runtime_error("close() failed");
								clients.erase(events[i].ident);

								std::cout << BMAG << "connection closed" << CRESET << std::endl;

							}

							break;

						case EVFILT_TIMER:

							timeout.tv_sec = 5;
							timeout.tv_nsec = 0;

							EV_SET(&changes, events[i].ident, EVFILT_TIMER, EV_DELETE, 0, 0, nullptr);
							if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

							timeout.tv_sec = 5;
							timeout.tv_nsec = 0;

							EV_SET(&changes, events[i].ident, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
							if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

							clients[events[i].ident].response.handle(clients[events[i].ident].request, clients[events[i].ident].server, config, true);

							timeout.tv_sec = 5;
							timeout.tv_nsec = 0;

							EV_SET(&changes, events[i].ident, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
							if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

							std::cout << BMAG << "client timed out" << CRESET << std::endl;

							break;

					}
				}

			} catch (std::exception &e) {
				std::cerr << e.what() << std::endl;
			}

		}

	}

}

void listen(Server const &server) {

	std::cout << "Starting server: '" << server.servername << '\'' << std::endl;

	for (std::vector<Address>::const_iterator address = server.addresses.begin(); address != server.addresses.end(); address++) {

		int opt = 1;
		if (setsockopt(address->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
			throw std::runtime_error("setsockopt() failed");
		}

		if (bind(address->fd, (struct sockaddr *) &address->address, sizeof(address->address)) == -1) {
			throw std::runtime_error("bind() failed");
		}

		if (listen(address->fd, MAX_EVENTS) == -1) {
			throw std::runtime_error("listen() failed");
		}

		std::cout << "Listening on port " << address->port << "..." << std::endl;

	}

}

void cleanup(Config const & config) {
	std::cout << "Cleaning up..." << std::endl;
	std::vector<Server> const & servers = config.getServers();
	for (std::vector<Server>::const_iterator server = servers.begin(); server != servers.end(); server++) {
		for (std::vector<Address>::const_iterator address = server->addresses.begin(); address != server->addresses.end(); address++) {
			std::cout << "Closing port " << address->port << "..." << std::endl;
			if (close(address->fd) == -1) {
				throw std::runtime_error("close() failed");
			}
		}
	}
}

int main(int argc, char ** argv, char **env) {

	// Check arguments
	if (argc > 2) {
		std::cerr << "Usage: " << ((argc) ? argv[0] : "webserv") << " <config_file>" << std::endl;
		return USAGE_FAILURE;
	}

	// Getting config
	Config config;
	try {
		if (argc != 2) std::cout << "No config file given, using default" << std::endl;
		config.load((argc == 2) ? argv[1] : "webserv.conf", env);
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
		std::vector<Server> const & servers = config.getServers();
		for (std::vector<Server>::const_iterator server = servers.begin(); server != servers.end(); server++) {
			listen(*server);
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