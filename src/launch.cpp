#include <iostream>
#include <Config/Config.hpp>

#ifdef __linux__

void launch(Config const &config) {
	(void) config;
	std::cout << "salut" << std::endl;
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
