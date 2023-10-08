#include <iostream>
#include <Config/Config.hpp>
#include <utils/utils.hpp>
#include <utils/Colors.hpp>
#include <Client.hpp>
#include <unistd.h>
#include <stdio.h>

#ifdef __linux__

#include <sys/select.h>

void launch(Config const &config) {

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
			if (address->fd > max_fd) {
				max_fd = address->fd;
			}
		}
	}

	timeval   timeout;
	socklen_t addrlen = sizeof(sockaddr_in);

	while (true) {

		fd_set tmp_read_fds = read_fds;
		fd_set tmp_write_fds = write_fds;
		fd_set tmp_except_fds = except_fds;

		timeout.tv_sec = TIMEOUT_S;
		timeout.tv_usec = 0;

		int num_ready_fds = select(max_fd + 1, &tmp_read_fds, &tmp_write_fds, &tmp_except_fds, &timeout);
		max_fd = 0;

		if (num_ready_fds == -1) {
			perror("socket");
			std::cerr << "select() failed" << std::endl;
			continue;
		}

		for (std::vector<Server>::const_iterator server = servers.begin(); server != servers.end(); server++) {
			for (std::vector<Address>::const_iterator address = server->addresses.begin(); address != server->addresses.end(); address++) {

				if (max_fd < address->fd) {
					max_fd = address->fd;
				}

				if (FD_ISSET(address->fd, &tmp_read_fds)) {
					// Handle read event for address->fd
					// ...

					int client_fd = accept(address->fd, (sockaddr *) &address->address, &addrlen);

					Client client;

					client.server = &*server;
					client.request.setFd(client_fd);
					client.response.setFd(client_fd);

					clients[client_fd] = client;

					FD_SET(client_fd, &read_fds);

					std::cout << "server rfds" << std::endl;

				}

				if (FD_ISSET(address->fd, &tmp_write_fds)) {
					// Handle write event for address->fd
					// ...

					std::cout << "server wfds" << std::endl;

				}

				if (FD_ISSET(address->fd, &tmp_except_fds)) {
					// Handle exception event for address->fd
					// ...

					std::cout << "server efds" << std::endl;

				}

			}
		}

		for (std::map<int, Client>::iterator client = clients.begin(); client != clients.end(); client++) {

			if (max_fd < client->first) {
				max_fd = client->first;
			}

			if (FD_ISSET(client->first, &tmp_write_fds)) {
				// Handle write event for client->first
				// ...

				client->second.response.write();

				std::cout << "client wfds" << std::endl;

			}

			if (FD_ISSET(client->first, &tmp_except_fds)) {
				// Handle exception event for client->first
				// ...

				std::cout << "client efds" << std::endl;

			}

			if (FD_ISSET(client->first, &tmp_read_fds)) {
				// Handle read event for client->first
				// ...

				if (client->second.request.read(client->second.server->max_client_body_size) == 0) {
					FD_CLR(client->first, &read_fds);
					FD_CLR(client->first, &write_fds);
					FD_CLR(client->first, &except_fds);
					clients.erase(client);

					std::cout << BMAG << "disconnect" << CRESET << std::endl;

				}

				else if (client->second.request.isFinished()) {
					client->second.request.parse(client->second.server);
					client->second.response.handle(client->second.request, client->second.server, config, false);
				}

				std::cout << "client rfds" << std::endl;

			}

		}

		// Check for socket events
		// for (int sockfd = 0; sockfd <= max_fd; sockfd++) {

		// 	if (FD_ISSET(sockfd, &tmp_read_fds)) {
		// 		// Handle read event for sockfd
		// 		// ...

		// 		std::cout << "rfds" << std::endl;

		// 	}

		// 	if (FD_ISSET(sockfd, &tmp_write_fds)) {
		// 		// Handle write event for sockfd
		// 		// ...

		// 		std::cout << "wfds" << std::endl;

		// 	}

		// 	if (FD_ISSET(sockfd, &tmp_except_fds)) {
		// 		// Handle exception event for sockfd
		// 		// ...

		// 		std::cout << "efds" << std::endl;

		// 	}

		// }

		// Handle other tasks and cleanup
		// ...

	}

}

#else

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
			EV_SET(&changes, address->fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
			if (kevent(kq, &changes, 1, NULL, 0, NULL) == -1) {
				throw std::runtime_error("kevent() failed");
			}
		}
	}

	timespec timeout;
	int nev;

	while (true) {

		timeout.tv_sec = 5;
		timeout.tv_nsec = 0;

		if ((nev = kevent(kq, NULL, 0, events, MAX_EVENTS, &timeout)) == -1) {
			std::cerr << "kevent() failed" << std::endl;
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
									std::cout << BYEL << "status 413" << CRESET << std::endl;
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

								std::cout << BMAG << "connection closed" << CRESET << std::endl;

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

#endif
