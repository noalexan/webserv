#include <iostream>
#include <fstream>

#include "WebservConfig.hpp"

int main(int argc, char** argv) {
	std::cout << "Hello World!" << std::endl;

	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config file>" << std::endl;
		return 1;
	}

	WebservConfig config(argv[1]);

	std::cout << std::endl;
	for (std::deque<Server>::iterator it = config.servers().begin(); it != config.servers().end(); it++) {
		std::cout << "Server: " << it->host << ":" << it->port << std::endl;
		for (std::map<std::string, Location>::iterator it2 = it->locations.begin(); it2 != it->locations.end(); it2++) {
			std::cout << "\tLocation: " << it2->first << std::endl;
			std::cout << "\t\troot: " << it2->second.root << std::endl;
			std::cout << "\t\tindexes: ";
			for (std::deque<std::string>::iterator it3 = it2->second.indexes.begin(); it3 != it2->second.indexes.end(); it3++) {
				std::cout << *it3 << " ";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;

	std::cout << "Goodbye World!" << std::endl;

	return 0;
}

// Authors : Charly Tardy, Marwan Ayoub, Noah Alexandre
// Version : 0.2.0
