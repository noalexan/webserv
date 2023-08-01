#include <iostream>
#include <unistd.h>
#include <sys/event.h>
#include <ExitCode.hpp>
#include <Config/Config.hpp>
#include <Request/Request.hpp>
#include <Response/Response.hpp>

#define BUFFER_SIZE 1024

// je me place ici j'en rien a foutre



class Client {
	
	private :

		int _socketDescriptor;

	public :

		Client(int socketDescriptor) : _socketDescriptor(socketDescriptor) {}
		int getSocketDescriptor() { 
			return _socketDescriptor;
		}
		void closeConnection() {
			close(_socketDescriptor);
		}
};


void launch(Config const &config) {
    const int MAX_EVENTS = 100;
    int kq = kqueue();
    if (kq == -1) {
        throw std::runtime_error("Error: kqueue() failed");
    }

    struct kevent events[MAX_EVENTS];
    std::map<int, Client> clientsMap; // Le std::map pour stocker les clients

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
                clientsMap.erase(fd); // Supprimer le client du std::map lorsqu'il se déconnecte
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

            char buffer[BUFFER_SIZE];
            ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE);

            if (bytes_read == -1) {
                std::cerr << "Error: read() failed" << std::endl;
                if (close(client_fd) == -1) {
                    std::cerr << "Error: close() failed" << std::endl;
                }
                continue;
            }

            if (bytes_read == 0) {
                std::cout << "Client disconnected" << std::endl;
                if (close(client_fd) == -1) {
                    std::cerr << "Error: close() failed" << std::endl;
                }
                continue;
            }

            if (bytes_read == BUFFER_SIZE) {
                std::cerr << "Error: buffer full" << std::endl;
                if (close(client_fd) == -1) {
                    std::cerr << "Error: close() failed" << std::endl;
                }
                continue;
            }

            std::cout << "Received " << bytes_read << " bytes" << std::endl;

            buffer[bytes_read] = '\0';

            for (int i = 0; i < bytes_read; i++) {
                if (buffer[i] == '\r') {
                    std::cout << "\e[31m\\r\e[0m";
                } else if (buffer[i] == '\n') {
                    std::cout << "\e[31m\\n\e[0m" << std::endl;
                } else {
                    std::cout << "\x1b[32m" << buffer[i] << "\x1b[0m";
                }
            }

            std::string _(buffer, bytes_read);
            Request request(_, server);
			clientsMap[client_fd] = Client(client_fd);
            if (events[i].flags & EV_EOF) {
                clientsMap[client_fd].closeConnection();
                clientsMap.erase(client_fd);
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
