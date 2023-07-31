#include <Config/Config.hpp>

#include <iostream>
#include <fstream>

#include <unistd.h>

static std::string getWord(std::string &line) {
	std::string word;
	line.erase(0, line.find_first_not_of(" \t\n"));
	word = line.substr(0, line.find_first_of(" \t\n;"));
	line.erase(0, word.length());
	return word;
}

Config::Config() {}

Config::Config(char const *ConfigFileName) {

	std::ifstream ConfigFile(ConfigFileName);
	if (!ConfigFile.is_open()) {
		throw std::runtime_error("Error: cannot open config file");
	}

	_contentTypes[".txt"] = "text/plain";
	_contentTypes[".html"] = "text/html";
	_contentTypes[".css"] = "text/css";
	_contentTypes[".js"] = "text/javascript";
	_contentTypes[".xml"] = "text/xml";
	_contentTypes[".json"] = "application/json";
	_contentTypes[".jpg"] = "image/jpeg";
	_contentTypes[".png"] = "image/png";
	_contentTypes[".gif"] = "image/gif";
	_contentTypes[".ico"] = "image/x-icon";
	_contentTypes[".mp4"] = "video/mp4";

	std::string line;
	int line_number = 0;
	while (std::getline(ConfigFile, line)) {

		line_number++;

		line.erase(0, line.find_first_not_of(" \t\n"));
		line.erase(line.find_last_not_of(" \t\n") + 1);

		if (line.empty() || line[0] == '#') {
			continue;
		}

		if (line[line.length() - 1] != ';' && line[line.length() - 1] != '{' && line[line.length() - 1] != '}') {
			ConfigFile.close();
			throw std::runtime_error("Error: " + std::to_string(line_number) + ": missing ';' at the end of line");
		}

		std::string word = getWord(line);

		if (word == "http") {

			if (getWord(line) != "{" || getWord(line).length()) {
				ConfigFile.close();
				throw std::runtime_error("Error: " + std::to_string(line_number) + ": invalid line");
			}

			std::cout << "Entering http block" << std::endl;

			while (std::getline(ConfigFile, line)) {

				line_number++;

				line.erase(0, line.find_first_not_of(" \t\n"));
				line.erase(line.find_last_not_of(" \t\n") + 1);

				if (line == "}") {
					break;
				}

				if (line.empty() || line[0] == '#') {
					continue;
				}

				if (line[line.length() - 1] != ';' && line[line.length() - 1] != '{' && line[line.length() - 1] != '}') {
					ConfigFile.close();
					throw std::runtime_error("Error: " + std::to_string(line_number) + ": missing ';' at the end of line");
				}

				std::string word = getWord(line);

				if (word == "server") {

					if (getWord(line) != "{" || getWord(line).length()) {
						ConfigFile.close();
						throw std::runtime_error("Error: " + std::to_string(line_number) + ": invalid line");
					}

					std::cout << "Entering server block" << std::endl;

					struct Server server;
					struct Location mainLocation;

					mainLocation.directoryListing = true;

					while (std::getline(ConfigFile, line)) {

						line_number++;

						line.erase(0, line.find_first_not_of(" \t\n"));
						line.erase(line.find_last_not_of(" \t\n") + 1);

						if (line == "}") {
							break;
						}

						if (line.empty() || line[0] == '#') {
							continue;
						}

						if (line[line.length() - 1] != ';' && line[line.length() - 1] != '{' && line[line.length() - 1] != '}') {
							ConfigFile.close();
							throw std::runtime_error("Error: " + std::to_string(line_number) + ": missing ';' at the end of line");
						}

						std::string word = getWord(line);

						if (word == "listen") {
							std::string listen = getWord(line);
							try {
								server.port = std::stoi(listen);
							} catch (std::exception &) {
								ConfigFile.close();
								throw std::runtime_error("Error: " + std::to_string(line_number) + ": invalid port");
							}
							std::cout << "port: " << server.port << std::endl;
						} else if (word == "server_name") {
							std::string host;
							std::cout << "hosts: " << line << std::endl;
							while ((host = getWord(line)).length()) server.hosts.push_back(host);
						} else if (word == "root") {

							line.erase(0, line.find_first_not_of(" \t"));
							std::string root = line.substr(0, line.find_last_of(';'));

							if (root[root.length() - 1] != '/') root += '/';

							std::cout << "root: '" << root << "'" << std::endl;

							if (access(root.c_str(), F_OK) == -1) {
								ConfigFile.close();
								throw std::runtime_error("Error: " + std::to_string(line_number) + ": root directory does not exist");
							}

							mainLocation.root = root;
						} else if (word == "directory_listing") {
							mainLocation.directoryListing = ((getWord(line) == "on") ? true : false);
						} else if (word == "index") {
							std::string index;
							std::cout << "index: " << line << std::endl;
							while ((index = getWord(line)).length()) mainLocation.indexes.push_back(index);
						} else if (word == "location") {

							std::string location_path = getWord(line);

							if (getWord(line) != "{" || getWord(line).length()) {
								ConfigFile.close();
								throw std::runtime_error("Error: " + std::to_string(line_number) + ": invalid line");
							}

							if (location_path == "/") {
								ConfigFile.close();
								throw std::runtime_error("Error: " + std::to_string(line_number) + ": for location /, please specify root and index in server block");
							}

							if (server.locations.find(location_path) != server.locations.end()) {
								ConfigFile.close();
								throw std::runtime_error("Error: " + std::to_string(line_number) + ": duplicate location");
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

								if (line.empty() || line[0] == '#') {
									continue;
								}

								if (line[line.length() - 1] != ';') {
									ConfigFile.close();
									throw std::runtime_error("Error: " + std::to_string(line_number) + ": missing ';' at the end of line");
								}

								std::string word = getWord(line);

								if (word == "root") {
									line.erase(0, line.find_first_not_of(" \t"));
									std::string root = line.substr(0, line.find_last_of(';'));
									if (root[root.length() - 1] != '/') root += '/';
									std::cout << "root: '" << root << "'" << std::endl;

									if (access(root.c_str(), F_OK) == -1) {
										ConfigFile.close();
										throw std::runtime_error("Error: " + std::to_string(line_number) + ": root directory does not exist");
									}

									location.root = root;
								} else if (word == "index") {
									std::string index;
									std::cout << "index: " << line << std::endl;
									while ((index = getWord(line)).length()) location.indexes.push_back(index);
								} else {
									ConfigFile.close();
									throw std::runtime_error("Error: " + std::to_string(line_number) + ": unrecognized location rule");
								}

							}

							server.locations[location_path] = location;

							std::cout << "Exiting location block" << std::endl;

						} else {
							ConfigFile.close();
							throw std::runtime_error("Error: " + std::to_string(line_number) + ": unrecognized server rule");
						}

					}

					if (mainLocation.root.empty()) {
						ConfigFile.close();
						throw std::runtime_error("Error: " + std::to_string(line_number) + ": no root specified in server block");
					}

					if (mainLocation.indexes.empty()) {
						ConfigFile.close();
						throw std::runtime_error("Error: " + std::to_string(line_number) + ": no index specified in server block");
					}

					server.locations["/"] = mainLocation;

					server.address.sin_family = AF_INET;
					server.address.sin_port = htons(server.port);
					server.address.sin_addr.s_addr = htonl(INADDR_ANY);

					server.fd = socket(AF_INET, SOCK_STREAM, 0);

					if (server.fd == -1) {
						ConfigFile.close();
						throw std::runtime_error("Error: " + std::to_string(line_number) + ": socket() failed");
					}

					_servers.push_back(server);

					std::cout << "Exiting server block" << std::endl;

				} else {
					ConfigFile.close();
					throw std::runtime_error("Error: " + std::to_string(line_number) + ": unrecognized rule");
				}

			}

			std::cout << "Exiting http block" << std::endl;

		} else {
			ConfigFile.close();
			throw std::runtime_error("Error: " + std::to_string(line_number) + ": unrecognized rule");
		}

	}

	ConfigFile.close();

}
