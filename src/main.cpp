#include <iostream>
#include <unistd.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <utils/ExitCode.hpp>
#include <utils/Colors.hpp>
#include <Config/Config.hpp>
#include <Client.hpp>

#define BUFFER_SIZE 1 << 9
#define MAX_EVENTS 1 << 10
#define TIMEOUT_US 1000000 // ? temporary

void launch(Config const &config) {

	int kq = kqueue();

	if (kq == -1) {
		throw std::runtime_error("kqueue() failed");
	}

	struct kevent events[MAX_EVENTS], changes, tm;

	std::map<int, Server> const & servers = config.getServers();
	std::map<int, Client> clients;

	for (std::map<int, Server>::const_iterator server = servers.begin(); server != servers.end(); server++) {
		EV_SET(&changes, server->second.fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
		if (kevent(kq, &changes, 1, nullptr, 0, nullptr) == -1) {
			throw std::runtime_error("kevent() failed");
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
					std::cout << "disconnect" << std::endl;
					uintptr_t const & fd = events[i].ident;
					EV_SET(&changes, fd, events[i].filter, EV_DELETE, 0, 0, nullptr);

					if (kevent(kq, &changes, 1, nullptr, 0, nullptr) == -1) {
						throw std::runtime_error("kevent() failed");
					}

					if (close(events[i].ident) == -1) {
						throw std::runtime_error("close() failed");
					}

					clients.erase(events[i].ident);

					continue;
				}

				if (servers.find(events[i].ident) != servers.end()) {
					std::cout << "new connection" << std::endl;

					sockaddr_in client_address;
					socklen_t client_address_len = sizeof(client_address);
					int client_fd;

					if ((client_fd = accept(servers.at(events[i].ident).fd, (struct sockaddr *) &client_address, &client_address_len)) == -1) {
						throw std::runtime_error("accept() failed");
					}

					fcntl(client_fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
					EV_SET(&changes, client_fd, EVFILT_READ, EV_ADD, 0, 0, nullptr); // je t'ai vu jouer a la marelle

					timeout.tv_sec = 5; // sautillant sur le macadame
					timeout.tv_nsec = 0; // te voici maintenant jouvancelle

					if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) { // dans un corps pas tout a fait femme
						throw std::runtime_error("kevent() failed"); // je vais t'apprendre un jeu extra
					} // qu'il ne faudra pas repeter

					EV_SET(&tm, client_fd, EVFILT_TIMER, EV_ADD, NOTE_USECONDS, TIMEOUT_US, nullptr);
					if (kevent(kq, &tm, 1, nullptr, 0, nullptr) == -1) {
						throw std::runtime_error("kevent() failed (EVFILT_TIMER)");
					}

					Client client; // la regle est simple tu verras

					client.server = &servers.at(events[i].ident); // il suffit juste de s'embrasser
					client.request.setFd(client_fd);
					client.response.setFd(client_fd);

					clients[client_fd] = client;

					std::cout << "client created" << std::endl;

					continue;
				}

				if (clients.find(events[i].ident) != clients.end()) {
					switch (events[i].filter) {
						case EVFILT_READ:
							clients[events[i].ident].request.read();

							// std::cout << BRED << "evfilt read from " << events[i].ident << CRESET << std::endl;
							if (clients[events[i].ident].request.isFinished()) {
								EV_SET(&changes, events[i].ident, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);

								timeout.tv_sec = 5; // привет
								timeout.tv_nsec = 0;

								if (kevent(kq, &changes, 1, nullptr, 0, &timeout) == -1) {
									throw std::runtime_error("kevent() failed");
								}

								clients[events[i].ident].request.parse(clients[events[i].ident].server);
								clients[events[i].ident].response.handle(clients[events[i].ident].request, clients[events[i].ident].server);
							}

							break;

						case EVFILT_WRITE:
							clients[events[i].ident].response.write();

							if (clients[events[i].ident].response.isFinished()) {
								uintptr_t const & fd = events[i].ident;

								EV_SET(&changes, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
								if (kevent(kq, &changes, 1, nullptr, 0, nullptr) == -1) {
									throw std::runtime_error("kevent() failed");
								}

								if (close(events[i].ident) == -1) {
									throw std::runtime_error("close() failed");
								}

								clients.erase(events[i].ident);

								std::cout << "connection closed" << std::endl;
							}

							break;

						case EVFILT_TIMER:
						{
							std::cout << UBLK << "MAMACITA CA MARCHE" << CRESET << std::endl;
							std::string	timeout_resp = clients[events[i].ident].request.getVersion() + ' ' + GATEWAY_TIMEOUT;
							send(events[i].ident, (timeout_resp).c_str(), timeout_resp.length(), 0);
							clients.erase(events[i].ident);
							break;
						}

						default:
							std::cout << "unhandled filter" << std::endl;
					}

					continue;
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
		throw std::runtime_error("setsockopt() failed");
	}

	if (bind(server.fd, (struct sockaddr *) &server.address, sizeof(server.address)) == -1) {
		throw std::runtime_error("bind() failed");
	}

	if (listen(server.fd, MAX_EVENTS) == -1) {
		throw std::runtime_error("listen() failed");
	}

	std::cout << "Listening on port " << server.port << "..." << std::endl;

}

void cleanup(Config const &config) {
	std::cout << "Cleaning up..." << std::endl;
	for (std::map<int, Server>::const_iterator server = config.getServers().begin(); server != config.getServers().end(); server++) {
		std::cout << "Closing port " << server->second.port << "..." << std::endl;
		if (close(server->first) == -1) {
			throw std::runtime_error("close() failed");
		}
	}
}

int main(int argc, char ** argv, char **env) {

	// Check arguments
	if (argc > 2) {
		std::cerr << "Usage: " << ((argc) ? argv[0] : "webserv") << " <config_file>" << std::endl;
		return USAGE_FAILURE;
	}

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		std::cerr << "signal() failed" << std::endl;
		return SIGNAL_FAILURE;
	}

	// Getting config
	Config config;
	try {
		config = Config((argc == 2) ? argv[1] : "webserv.conf", env);
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
		for (std::map<int, Server>::const_iterator server = config.getServers().begin(); server != config.getServers().end(); server++) {
			listen(server->second);
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