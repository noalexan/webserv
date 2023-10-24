#include <iostream>
#include <Config/Config.hpp>
#include <utils/utils.hpp>
#include <utils/Colors.hpp>
#include <Client.hpp>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

bool running = true;

void stop(int) {
	running = false;
	std::cout << std::endl;
}

#ifdef __linux__

#include <sys/select.h>

#define MAX(a,b) a < b ? b : a

void launch(Config const &config) {

	signal(SIGINT, stop);

	std::vector<Server> const &servers = config.getServers();
	std::map<int, Client> clients;

	fd_set read_fds;
	fd_set write_fds;
	fd_set except_fds;

	int max_fd = 0;

	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&except_fds);

	for (std::vector<Server>::const_iterator server = servers.begin(); server != servers.end(); server++) {
		for (std::vector<Address>::const_iterator address = server->addresses.begin(); address != server->addresses.end(); address++) {

			FD_SET(address->fd, &read_fds);
			FD_SET(address->fd, &write_fds);
			FD_SET(address->fd, &except_fds);

			if (address->fd > max_fd) {
				max_fd = address->fd;
			}

		}
	}

	socklen_t addrlen = sizeof(sockaddr_in);
	timeval tv;

	while (running) {

		fd_set tmp_read_fds   = read_fds;
		fd_set tmp_write_fds  = write_fds;
		fd_set tmp_except_fds = except_fds;

		tv.tv_sec = 0;
		tv.tv_usec = 100;

		if (select(max_fd + 1, &tmp_read_fds, &tmp_write_fds, &tmp_except_fds, &tv) == -1) {
			if (running) {
				std::cerr << timestamp() << "[!] select() failed" << std::endl;
			}
			continue;
		}

		max_fd = 0;

		for (std::vector<Server>::const_iterator server = servers.begin(); server != servers.end(); server++) {
			for (std::vector<Address>::const_iterator address = server->addresses.begin(); address != server->addresses.end(); address++) {

				max_fd = MAX(address->fd, max_fd);

				if (FD_ISSET(address->fd, &tmp_read_fds)) {

					int client_fd = accept(address->fd, (sockaddr *) &address->address, &addrlen);

					if (client_fd == -1) {
						std::cerr << timestamp() << "[!] accept() failed" << std::endl;
						continue;
					}

					fcntl(client_fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC);

					Client client;

					client.server = &*server;
					client.request.setFd(client_fd);
					client.response.setFd(client_fd);
					client.timestamp = time(NULL);

					clients[client_fd] = client;

					FD_SET(client_fd, &read_fds);

					std::cout << timestamp() << "new client (" << client_fd << ")" << std::endl;

				}

			}
		}

		for (std::map<int, Client>::const_iterator client = clients.begin(); client != clients.end(); client++) { // t'as une canette à l'horizontale
			max_fd = MAX(client->first, max_fd);
		}

		for (int client_fd = 0; client_fd <= max_fd; client_fd++) {

			if (clients.find(client_fd) == clients.end()) {
				continue;
			}

			Client & client = clients.at(client_fd);
			max_fd = MAX(client_fd, max_fd);

			try {

				if (FD_ISSET(client_fd, &read_fds) && difftime(time(NULL), client.timestamp) >= TIMEOUT_S) {
					client.response.handle(client.request, client.server, config, true);
					FD_CLR(client_fd, &read_fds);
					FD_SET(client_fd, &write_fds);

					std::cout << timestamp() << BMAG << "client timed out" << CRESET << std::endl;
				} else if (FD_ISSET(client_fd, &tmp_read_fds)) {

					if (client.request.read(client.server->max_client_body_size) == 0) { // EOF

						FD_CLR(client_fd, &read_fds);

						if (close(client_fd) == -1) throw std::runtime_error("close() failed");
						clients.erase(client_fd);

						std::cout << timestamp() << BMAG << "disconnect (" << client_fd << ")" << CRESET << std::endl;

					}

					else if (client.request.isFinished()) {

						if (client.request.isTooLarge()) {
							std::cout << timestamp() << BYEL << "status 413" << CRESET << std::endl;
							client.response.responseMaker(client.server, "413", PAYLOAD_TOO_LARGE);
						} else {
							try {
								client.request.parse(client.server);
							} catch (std::exception const &e) {
								std::cerr << timestamp() << "[!] " << e.what() << std::endl;
								std::cout << timestamp() << BYEL << "status 400" << CRESET << std::endl;
								client.response.responseMaker(client.server, "400", BAD_REQUEST);
							}
							try {
								client.response.handle(client.request, client.server, config, false);
							} catch (std::exception const &e) {
								std::cerr << timestamp() << "[!] " << e.what() << std::endl;
								std::cout << timestamp() << BYEL << "status 500" << CRESET << std::endl;
								client.response.responseMaker(client.server, "500", BAD_REQUEST);
							}
						}

						FD_CLR(client_fd, &read_fds);
						FD_SET(client_fd, &write_fds);
					}

				}

				else if (FD_ISSET(client_fd, &tmp_write_fds)) {

					client.response.write();

					if (client.response.isFinished()) {

						FD_CLR(client_fd, &write_fds);

						if (close(client_fd) == -1) throw std::runtime_error("close() failed");
						clients.erase(client_fd);

						std::cout << timestamp() << BMAG << "connection closed (" << client_fd << ")" << CRESET << std::endl;

					}

				}

			} catch (std::exception const & e) {
				std::cout << timestamp() << "client deleted" << std::endl;
				close(client_fd);
				FD_CLR(client_fd, &read_fds);
				if (FD_ISSET(client_fd, &write_fds)) FD_CLR(client_fd, &write_fds);
				clients.erase(client_fd);
				std::cerr << timestamp() << "[!] " << e.what() << std::endl;
			}

		}

	}

	for (std::map<int, Client>::const_iterator client = clients.begin(); client != clients.end(); client++) {
		std::cout << timestamp() << BMAG << "connection closed (" << client->first << ")" << CRESET << std::endl;
		close(client->first);
	}

}

#else

#include <sys/event.h>

void launch(Config const &config) {

	signal(SIGINT, stop);

	int kq = kqueue();

	if (kq == -1) {
		throw std::runtime_error("kqueue() failed");
	}

	struct kevent events[MAX_EVENTS], changes;

	std::vector<Server> const & servers = config.getServers();
	std::map<int, Client> clients;

	for (std::vector<Server>::const_iterator server = servers.begin(); server != servers.end(); server++) {
		for (std::vector<Address>::const_iterator address = server->addresses.begin(); address != server->addresses.end(); address++) {
			EV_SET(&changes, address->fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
			if (kevent(kq, &changes, 1, NULL, 0, NULL) == -1) {
				throw std::runtime_error("kevent() failed");
			}
		}
	}

	timespec timeout;
	int nev;

	while (running) {

		timeout.tv_sec = 5;
		timeout.tv_nsec = 0;

		if ((nev = kevent(kq, NULL, 0, events, MAX_EVENTS, &timeout)) == -1) {
			std::cerr << timestamp() << "[!] kevent() failed" << std::endl;
			continue;
		}

		for (int i = 0; i < nev; i++) {

			try {

				if (events[i].flags & EV_EOF) {

					timeout.tv_sec = 5;
					timeout.tv_nsec = 0;

					EV_SET(&changes, events[i].ident, events[i].filter, EV_DELETE, 0, 0, NULL);
					if (kevent(kq, &changes, 1, NULL, 0, &timeout) == -1) {
						throw std::runtime_error("kevent() failed");
					}

					if (close(events[i].ident) == -1) {
						throw std::runtime_error("close() failed");
					}

					clients.erase(events[i].ident);

					std::cout << timestamp() << BMAG << "disconnect" << CRESET << std::endl;

				}
				
				bool isServer = false;

				for (std::vector<Server>::const_iterator server = servers.begin(); server != servers.end() and not isServer; server++) {
					for (std::vector<Address>::const_iterator address = server->addresses.begin(); address != server->addresses.end() and not isServer; address++) {
						if (address->fd == (int) events[i].ident) {

							std::cout << timestamp() << "receiving request from server '" << server->servername << '\'' << std::endl;
							isServer = true;

							sockaddr_in client_address;
							socklen_t client_address_len = sizeof(sockaddr_in);
							int client_fd = accept(events[i].ident, (sockaddr *) &client_address, &client_address_len);

							if (client_fd == -1) {
								throw std::runtime_error("accept() failed");
							}

							fcntl(client_fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
							EV_SET(&changes, client_fd, EVFILT_READ, EV_ADD, 0, 0, NULL); // je t'ai vu jouer a la marelle

							timeout.tv_sec = 5; // sautillant sur le macadame
							timeout.tv_nsec = 0; // te voici maintenant jouvancelle

							if (kevent(kq, &changes, 1, NULL, 0, &timeout) == -1) { // dans un corps pas tout a fait femme
								throw std::runtime_error("kevent() failed"); // je vais t'apprendre un jeu extra
							} // qu'il ne faudra pas repeter

							timeout.tv_sec = 5; // la regle est simple tu verras
							timeout.tv_nsec = 0; // il suffit juste de s'embrasser

							EV_SET(&changes, client_fd, EVFILT_TIMER, EV_ADD, NOTE_SECONDS, TIMEOUT_S, NULL);
							if (kevent(kq, &changes, 1, NULL, 0, &timeout) == -1) {
								throw std::runtime_error("kevent() failed");
							}

							Client client;

							client.server = &*server;
							client.request.setFd(client_fd);
							client.response.setFd(client_fd);

							clients[client_fd] = client;

							std::cout << timestamp() << BGRN << "new client" << CRESET << std::endl;
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

								EV_SET(&changes, events[i].ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
								if (kevent(kq, &changes, 1, NULL, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

								timeout.tv_sec = 5;
								timeout.tv_nsec = 0;

								EV_SET(&changes, events[i].ident, EVFILT_TIMER, EV_DELETE, 0, 0, NULL);
								if (kevent(kq, &changes, 1, NULL, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

								timeout.tv_sec = 5;
								timeout.tv_nsec = 0;

								EV_SET(&changes, events[i].ident, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
								if (kevent(kq, &changes, 1, NULL, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

								if (clients[events[i].ident].request.isTooLarge()) {
									std::cout << timestamp() << BYEL << "status 413" << CRESET << std::endl;
									clients[events[i].ident].response.responseMaker(clients[events[i].ident].server, "413", PAYLOAD_TOO_LARGE);
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

								EV_SET(&changes, events[i].ident, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
								if (kevent(kq, &changes, 1, NULL, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");
								if (close(events[i].ident) == -1) throw std::runtime_error("close() failed");
								clients.erase(events[i].ident);

								std::cout << timestamp() << BMAG << "connection closed" << CRESET << std::endl;

							}
							break;

						case EVFILT_TIMER:

							timeout.tv_sec = 5;
							timeout.tv_nsec = 0;

							EV_SET(&changes, events[i].ident, EVFILT_TIMER, EV_DELETE, 0, 0, NULL);
							if (kevent(kq, &changes, 1, NULL, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

							timeout.tv_sec = 5;
							timeout.tv_nsec = 0;

							EV_SET(&changes, events[i].ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
							if (kevent(kq, &changes, 1, NULL, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

							clients[events[i].ident].response.handle(clients[events[i].ident].request, clients[events[i].ident].server, config, true);

							timeout.tv_sec = 5;
							timeout.tv_nsec = 0;

							EV_SET(&changes, events[i].ident, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
							if (kevent(kq, &changes, 1, NULL, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

							std::cout << timestamp() << BMAG << "client timed out" << CRESET << std::endl;
							break;

					}
				}

			} catch (std::exception &e) {
				std::cout << timestamp() << "client deleted" << std::endl;
				if (close(events[i].ident) == -1) {
					std::cerr << "[!] close() failed" << std::endl;
				}

				timeout.tv_sec = 5;
				timeout.tv_nsec = 0;

				EV_SET(&changes, events[i].ident, events[i].filter, EV_DELETE, 0, 0, NULL);
				if (kevent(kq, &changes, 1, NULL, 0, &timeout) == -1) throw std::runtime_error("kevent() failed");

				clients.erase(events[i].ident);
				std::cerr << timestamp() << "[!] " << e.what() << std::endl;
			}

		}

	}

	for (std::map<int, Client>::const_iterator client = clients.begin(); client != clients.end(); client++) {
		std::cout << timestamp() << BMAG << "connection closed (" << client->first << ")" << CRESET << std::endl;
		close(client->first);
	}

}

#endif
