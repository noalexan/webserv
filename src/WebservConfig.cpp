#include "WebservConfig.hpp"

#include <iostream>
#include <fstream>

static std::string getWord(std::string &line) {
	std::string word;
	line.erase(0, line.find_first_not_of(" \t\n"));
	word = line.substr(0, line.find_first_of(" \t\n;"));
	line.erase(0, word.length());
	return word;
}

WebservConfig::WebservConfig(char const *ConfigFileName) {

	std::ifstream ConfigFile(ConfigFileName);
	if (!ConfigFile.is_open()) {
		std::cerr << "Error: could not open config file " << ConfigFileName << std::endl;
		return;
	}

	std::string line;
	int line_number = 0;
	while (std::getline(ConfigFile, line)) {

		line_number++;

		line.erase(0, line.find_first_not_of(" \t\n"));
		line.erase(line.find_last_not_of(" \t\n") + 1);

		if (line.empty()) {
			continue;
		}

		if (line[0] == '#') {
			continue;
		}

		if (line[line.length() - 1] != ';' && line[line.length() - 1] != '{' && line[line.length() - 1] != '}') {
			std::cerr << "Error: " << line_number << ": missing ';' at the end of line" << std::endl;
			exit(1);
		}

		std::string word = getWord(line);

		if (word == "http") {

			if (getWord(line) != "{" || getWord(line).length()) {
				std::cerr << "Error: " << line_number << ": invalid line" << std::endl;
				exit(1);
			}

			std::cout << "Entering http block" << std::endl;

			while (std::getline(ConfigFile, line)) {

				line_number++;

				line.erase(0, line.find_first_not_of(" \t\n"));
				line.erase(line.find_last_not_of(" \t\n") + 1);

				if (line == "}") {
					break;
				}

				if (line.empty()) {
					continue;
				}

				if (line[0] == '#') {
					continue;
				}

				std::string word = getWord(line);

				if (word == "server") {

					if (getWord(line) != "{" || getWord(line).length()) {
						std::cerr << "Error: " << line_number << ": invalid line" << std::endl;
						exit(1);
					}

					std::cout << "Entering server block" << std::endl;

					struct Server server;

					while (std::getline(ConfigFile, line)) {

						line_number++;

						line.erase(0, line.find_first_not_of(" \t\n"));
						line.erase(line.find_last_not_of(" \t\n") + 1);

						if (line == "}") {
							break;
						}

						if (line.empty()) {
							continue;
						}

						if (line[0] == '#') {
							continue;
						}

						std::string word = getWord(line);

						if (word == "listen") {
							std::string listen = getWord(line);
							std::cout << "listen: " << listen << std::endl;
							server.port = std::stoi(listen);
						} else if (word == "server_name") {
							std::string server_name = getWord(line);
							std::cout << "server_name: " << server_name << std::endl;
							server.host = server_name;
						} else if (word == "location") {

							std::string location_path = getWord(line);

							if (getWord(line) != "{" || getWord(line).length()) {
								std::cerr << "Error: " << line_number << ": invalid line" << std::endl;
								exit(1);
							}

							std::cout << "Entering location block" << std::endl;
							std::cout << "Setting location for " << location_path << std::endl;

							struct Location location;

							while (std::getline(ConfigFile, line)) {

								line_number++;

								line.erase(0, line.find_first_not_of(" \t\n"));
								line.erase(line.find_last_not_of(" \t\n") + 1);

								if (line == "}") {
									break;
								}

								if (line.empty()) {
									continue;
								}

								if (line[0] == '#') {
									continue;
								}

								std::string word = getWord(line);

								if (word == "root") {
									std::string root = getWord(line);
									std::cout << "root: " << root << std::endl;
									location.root = root;
								} else if (word == "index") {
									std::string index;
									std::cout << "index: " << line << std::endl;
									while ((index = getWord(line)).length()) location.indexes.push_back(index);
								} else {
									std::cerr << "Error: " << line_number << ": unrecognized location rule" << std::endl;
									exit(1);
									continue;
								}

							}

							server.locations[location_path] = location;

							std::cout << "Exiting location block" << std::endl;

						} else {
							std::cerr << "Error: " << line_number << ": unrecognized server rule" << std::endl;
							exit(1);
							continue;
						}

					}

					_servers.push_back(server);

					std::cout << "Exiting server block" << std::endl;

				} else {
					std::cerr << "Error: " << line_number << ": unrecognized http rule" << std::endl;
					exit(1);
					continue;
				}

			}

			std::cout << "Exiting http block" << std::endl;

		} else {
			std::cerr << "Error: " << line_number << ": unrecognized rule" << std::endl;
			exit(1);
			continue;
		}

	}

	ConfigFile.close();

}
