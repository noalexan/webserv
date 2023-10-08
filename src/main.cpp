#include <iostream>

#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <Config/Config.hpp>
#include <Client.hpp>

#include <utils/ExitCode.hpp>
#include <utils/Colors.hpp>

#include <signal.h>

#define MAX_EVENTS 1024
#define TIMEOUT_S 2

void launch(Config const &config);

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

	// Ignore SIGPIPE
 	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
 		std::cerr << "signal() failed" << std::endl;
 		return SIGNAL_FAILURE;
 	}

	// Check arguments
	if (argc > 2) {
		std::cerr << "Usage: " << ((argc) ? argv[0] : "webserv") << " <config_file>" << std::endl;
		return USAGE_FAILURE;
	}

	// Getting config
	Config config;
	try {
		if (argc != 2) std::cout << "No config file given, using default" << std::endl;
		config.load(argc == 2 ? argv[1] : "webserv.conf", env);
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

// Authors : Marwan Ayoub, Noah Alexandre
// Version : 1.0.0
