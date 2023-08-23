#include <Config/Config.hpp>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

enum BLOCK_LVL {
	BEGIN,
	HTTP_BLOCK,
	SERVER_BLOCK,
	LOCATION_BLOCK,
	END,
};

static bool getWord(std::string & line, std::string & word) {
	line.erase(0, line.find_first_not_of(" \t"));
	if (line.empty()) return false;
	word = line.substr(0, line.find_first_of(" \t"));
	line.erase(0, word.length());
	return true;
}

static int parseInt(std::string const & str) {
	if (str.length() >= 6) return 0;
	for (std::string::const_iterator it = str.begin(); it != str.end(); it++)
		if ('0' > *it or *it > '9') return 0;
	return std::stoi(str);
}

static bool isDir(std::string const & str) {
	struct stat path_stat;
	stat(str.c_str(), &path_stat);
	return S_ISDIR(path_stat.st_mode);
}

static bool isFile(std::string const & str) {
	struct stat path_stat;
	stat(str.c_str(), &path_stat);
	return S_ISREG(path_stat.st_mode);
}

static bool isExecutable(std::string const & str) {
	return access(str.c_str(), X_OK) + 1;
}

Config::Config() {}

Config::Config(char const *ConfigFileName) {

	std::ifstream file(ConfigFileName);
	if (not file.good()) {
		throw std::runtime_error("Unable to open file");
	}

	_contentTypes[".txt"]	= "text/plain";
	_contentTypes[".html"]	= "text/html";
	_contentTypes[".css"]	= "text/css";
	_contentTypes[".js"]	= "text/javascript";
	_contentTypes[".xml"]	= "text/xml";
	_contentTypes[".json"]	= "application/json";
	_contentTypes[".jpg"]	= "image/jpeg";
	_contentTypes[".png"]	= "image/png";
	_contentTypes[".gif"]	= "image/gif";
	_contentTypes[".ico"]	= "image/x-icon";
	_contentTypes[".mp4"]	= "video/mp4";

	Server server;
	Location location;

	location.directory_listing = false;

	std::string line;
	std::string save;
	unsigned int line_number = 0;
	unsigned short blocklvl = BEGIN;
	while (not save.empty() or (std::getline(file, line) and ++line_number)) {

		line = save + line;

		line.erase(0, line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);

		if (line.empty() or line[0] == '#') continue;

		save = line.substr(line.find_first_of(";{}") + 1);
		line.erase(line.find_first_of(";{}") + 1);

		char endl_char = line[line.size() - 1];
		line.pop_back();

		if (endl_char != ';' and endl_char != '{') {
			if (endl_char != '}') throw std::runtime_error(std::to_string(line_number) + ": '}' isn't a valid end of rule");
			if (not line.empty()) throw std::runtime_error(std::to_string(line_number) + ": Expected a ';' or '{' at end of rules");
		}

		std::cout << line << " '" << endl_char << "'" << std::endl;

		std::string word;
		getWord(line, word);

		switch (blocklvl) {

			case BEGIN:
				if (word == "http") {
					if (getWord(line, word)) throw std::runtime_error(std::to_string(line_number) + ": Invalid line");
					if (endl_char != '{') throw std::runtime_error(std::to_string(line_number) + ": 'http' rule must be a block");
					blocklvl = HTTP_BLOCK;
				} else throw std::runtime_error(std::to_string(line_number) + ": " + word + ": Unrecognized rule");
				break;

			case HTTP_BLOCK:
				if (word.empty() and endl_char == '}') blocklvl = END;
				else if (word == "server") {
					if (getWord(line, word)) throw std::runtime_error(std::to_string(line_number) + ": Invalid line");
					if (endl_char != '{') throw std::runtime_error(std::to_string(line_number) + ": 'server' rule must be a block");
					blocklvl = SERVER_BLOCK;
				} else throw std::runtime_error(std::to_string(line_number) + ": " + word + ": Unrecognized http rule");
				break;

			case SERVER_BLOCK:
				if (word.empty() and endl_char == '}') {
					if (server.port == 0) throw std::runtime_error(std::to_string(line_number) + ": No port specified");
					server.address = (sockaddr_in) {.sin_family = AF_INET, .sin_port = htons(server.port), .sin_addr.s_addr = htonl(INADDR_ANY)};
					server.fd = socket(AF_INET, SOCK_STREAM, 0);
					_servers.push_back(server);
					server.port = 0;
					blocklvl = HTTP_BLOCK;
				} else if (word == "listen") {
					if (endl_char == '{') throw std::runtime_error(std::to_string(line_number) + ": 'listen' mustn't be a block");
					if (not getWord(line, word) or getWord(line, word)) throw std::runtime_error(std::to_string(line_number) + ": 'listen' expect an argument");
					if ((server.port = parseInt(word)) == 0) throw std::runtime_error(std::to_string(line_number) + ": Invalid port");
				} else if (word == "server_name") {
					if (endl_char == '{') throw std::runtime_error(std::to_string(line_number) + ": 'server_name' mustn't be a block");
					if (not getWord(line, word)) throw std::runtime_error(std::to_string(line_number) + ": Invalid line");
					do { server.hosts.push_back(word); } while (getWord(line, word));
				} else if (word == "location") {
					if (endl_char != '{') throw std::runtime_error(std::to_string(line_number) + ": location must be a block");
					if (not getWord(line, location.uri) or getWord(line, word)) throw std::runtime_error(std::to_string(line_number) + ": location require uri as argument");
					blocklvl = LOCATION_BLOCK;
				} else if (word == "max_client_body_size") {
					if (not getWord(line, word) or getWord(line, word)) throw std::runtime_error(std::to_string(line_number) + ": 'max_client_body_size' expect an argument");
					if ((server.max_client_body_size = std::stol(word)) == 0) throw std::runtime_error(std::to_string(line_number) + ": Invalid size");
				} else if (word == "cgi") {
					std::string executable;
					if (not getWord(line, word) or not getWord(line, executable) or getWord(line, word)) throw std::runtime_error(std::to_string(line_number) + ": 'cgi' expect two arguments");
					if (not isFile(executable) or not isExecutable(executable)) throw std::runtime_error(std::to_string(line_number) + ": unable to execute (" + executable + ")");
					server.cgi[word] = executable;
				} else if (word == "upload") {
					if (not getWord(line, word)) throw std::runtime_error(std::to_string(line_number) + ": 'upload' expect two arguments");
					line = line.substr(line.find_first_of(' ') + 1);
					if (not isDir(line)) throw std::runtime_error(std::to_string(line_number) + ": unable to access directory (" + line + ")");
					server.uploads[word] = line;
				} else throw std::runtime_error(std::to_string(line_number) + ": " + word + ": Unrecognized server rule");
				break;

			case LOCATION_BLOCK:
				if (word.empty() and endl_char == '}') {
					if (location.root.size() == 0) throw std::runtime_error(std::to_string(line_number) + ": No root specified");
					if (location.indexes.size() == 0) throw std::runtime_error(std::to_string(line_number) + ": No index specified");
					if (server.locations.find(location.uri) != server.locations.end()) throw std::runtime_error(std::to_string(line_number) + ": Duplicated location uri");
					server.locations[location.uri] = location;
					blocklvl = SERVER_BLOCK;
				} else if (word == "root") {
					if (endl_char == '{') throw std::runtime_error(std::to_string(line_number) + ": 'root' mustn't be a block");
					location.root = line.substr(line.find_first_not_of(" \t"));
					if (location.root.length() == 0) throw std::runtime_error(std::to_string(line_number) + ": no root");
					if (location.root[location.root.length() - 1] != '/') location.root += '/';
					if (not isDir(location.root)) throw std::runtime_error(std::to_string(line_number) + ": Unable to found root (" + location.root + ')');
				} else if (word == "index") {
					if (endl_char == '{') throw std::runtime_error(std::to_string(line_number) + ": 'index' mustn't be a block");
					if (not getWord(line, word)) throw std::runtime_error(std::to_string(line_number) + ": Invalid line");
					do { location.indexes.push_back(word); } while (getWord(line, word));
				} else if (word == "directory_listing") {
					if (endl_char == '{') throw std::runtime_error(std::to_string(line_number) + ": 'directory listing' mustn't be a block");
					if (not getWord(line, word) or getWord(line, word)) throw std::runtime_error(std::to_string(line_number) + ": invalid line");
					if (word != "on" and word != "off") throw std::runtime_error(std::to_string(line_number) + ": 'directory_listing' argument must be 'on' or 'off'");
					location.directory_listing = (word == "on");
				} else throw std::runtime_error(std::to_string(line_number) + ": " + word + ": Unrecognized location rule");
				break;

			case END:
				throw std::runtime_error(std::to_string(line_number) + ": off-block line");

		}

	}

	if (blocklvl != END) {
		throw std::runtime_error("Expected a closing brace at end of block");
	}

	file.close();
}
