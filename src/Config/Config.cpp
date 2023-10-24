#include <Config/Config.hpp>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/utils.hpp>
#include <cstdlib>

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
	return atoi(str.c_str());
}

static bool isDir(std::string const & str) {
	struct stat path_stat;
	stat(str.c_str(), &path_stat);
	return S_ISDIR(path_stat.st_mode);
}

static bool isFile(std::string const & str) {
	struct stat path_stat;
	stat(str.c_str(), &path_stat);
	return S_ISREG(path_stat.st_mode) && access(str.c_str(), R_OK) == 0;
}

static bool isExecutable(std::string const & str) {
	return access(str.c_str(), X_OK) + 1;
}

Config::Config() {
	_contentTypes["txt"]	= "text/plain";
	_contentTypes["html"]	= "text/html";
	_contentTypes["css"]	= "text/css";
	_contentTypes["js"]		= "text/javascript";
	_contentTypes["xml"]	= "text/xml";
	_contentTypes["json"]	= "application/json";
	_contentTypes["jpg"]	= "image/jpeg";
	_contentTypes["png"]	= "image/png";
	_contentTypes["gif"]	= "image/gif";
	_contentTypes["ico"]	= "image/x-icon";
	_contentTypes["mp4"]	= "video/mp4";
	_contentTypes["mp3"]	= "audio/mp3";
}

void Config::load(char const *ConfigFileName, char **env) {

	std::ifstream file(ConfigFileName);
	if (not file.good()) {
		throw std::runtime_error("Unable to open file");
	}

	Server server;
	Location location;

	server.env = env;

	std::string line;
	std::string save;
	unsigned int line_number = 0;
	unsigned short blocklvl = BEGIN;
	while (not save.empty() or (std::getline(file, line) and ++line_number)) {

		line = save + line;

		line.erase(0, line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);

		if (line.empty() or line[0] == '#') {
			line.clear();
			save.clear();
			continue;
		}

		size_t pos = line.find_first_of(";{}");
		if (pos != std::string::npos && pos != line.length()) {
			save = line.substr(pos + 1);
			line.erase(pos + 1);
		}

		char endl_char = line[line.length() - 1];
		line = line.substr(0, line.size() - 1);

		// std::cout << line << " '" << endl_char << '\'' << std::endl;

		if (endl_char != ';' and endl_char != '{' and (not line.empty() || endl_char != '}')) {
			throw std::runtime_error(SSTR(line_number) + ": Expected a ';' or '{' at end of rules");
		}

		std::string word;
		getWord(line, word);

		switch (blocklvl) {

			case BEGIN:
				if (word == "http") {
					if (getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": Invalid line");
					if (endl_char != '{') throw std::runtime_error(SSTR(line_number) + ": 'http' rule must be a block");
					blocklvl = HTTP_BLOCK;
				} else throw std::runtime_error(SSTR(line_number) + ": " + word + ": Unrecognized rule");
				break;

			case HTTP_BLOCK:
				if (word.empty() and endl_char == '}') blocklvl = END;
				else if (word == "server") {
					if (getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": Invalid line");
					if (endl_char != '{') throw std::runtime_error(SSTR(line_number) + ": 'server' rule must be a block");
					server.uploads.clear(); server.pages.clear(); server.cgi.clear(); server.redirect.clear();
					server.servername.clear(); server.locations.clear(); server.addresses.clear();
					server.max_client_body_size = 0;
					blocklvl = SERVER_BLOCK;
				} else throw std::runtime_error(SSTR(line_number) + ": " + word + ": Unrecognized http rule");
				break;

			case SERVER_BLOCK:
				if (word.empty() and endl_char == '}') {
					if (server.addresses.size() == 0) throw std::runtime_error(SSTR(line_number) + ": No port specified");
					_servers.push_back(server);
					blocklvl = HTTP_BLOCK;
				} else if (word == "listen") {
					if (endl_char == '{') throw std::runtime_error(SSTR(line_number) + ": 'listen' mustn't be a block");
					if (not getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": Invalid line");
					do {
						Address address;
						address.port = parseInt(word);
						if (address.port == -1) throw std::runtime_error(SSTR(line_number) + ": invalid port");
						address.address.sin_family = AF_INET;
						address.address.sin_port = htons(address.port);
						address.address.sin_addr.s_addr = htonl(INADDR_ANY);
						address.fd = socket(AF_INET, SOCK_STREAM, 0);
						server.addresses.push_back(address);
					} while (getWord(line, word));
				} else if (word == "server_name") {
					if (endl_char == '{') throw std::runtime_error(SSTR(line_number) + ": 'server_name' mustn't be a block");
					line.erase(0, line.find_first_not_of(" \t"));
					line.erase(line.find_last_not_of(" \t") + 1);
					server.servername = line;
				} else if (word == "location") {
					if (endl_char != '{') throw std::runtime_error(SSTR(line_number) + ": location must be a block");
					if (not getWord(line, location.uri) or getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": location require uri as argument");
					location.directory_listing = true;
					blocklvl = LOCATION_BLOCK;
				} else if (word == "max_client_body_size") {
					if (not getWord(line, word) or getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": 'max_client_body_size' expect an argument");
					if ((server.max_client_body_size = atol(word.c_str())) == 0) throw std::runtime_error(SSTR(line_number) + ": Invalid size");
				} else if (word == "cgi") {
					if (not getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": 'cgi' expect two arguments");
					line.erase(0, line.find_first_not_of(" \t"));
					line.erase(line.find_last_not_of(" \t") + 1);
					if (not isFile(line) or not isExecutable(line)) throw std::runtime_error(SSTR(line_number) + ": unable to execute (" + line + ")");
					if (server.cgi.find(word) != server.cgi.end()) throw std::runtime_error(SSTR(line_number) + ": conflict with cgi");
					server.cgi[word] = line;
				} else if (word == "upload") {
					if (not getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": 'upload' expect two arguments");
					line.erase(0, line.find_first_not_of(" \t"));
					line.erase(line.find_last_not_of(" \t") + 1);
					if (not isDir(line)) throw std::runtime_error(SSTR(line_number) + ": unable to access directory (" + line + ")");
					if (server.uploads.find(word) != server.uploads.end()) throw std::runtime_error(SSTR(line_number) + ": conflict with uploads");
					server.uploads[word] = line;
				} else if (word == "pages") {
					if (not getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": 'error' expect two arguments");
					line.erase(0, line.find_first_not_of(" \t"));
					line.erase(line.find_last_not_of(" \t") + 1);
					if (not isFile(line)) throw std::runtime_error(SSTR(line_number) + ": unable to access file (" + line + ")");
					if (server.pages.find(word) != server.pages.end()) throw std::runtime_error(SSTR(line_number) + ": conflict with pages");
					server.pages[word] = line;
				} else if (word == "redirect") {
					if (not getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": 'redirect' expect two arguments");
					line.erase(0, line.find_first_not_of(" \t"));
					line.erase(line.find_last_not_of(" \t") + 1);
					if (server.redirect.find(word) != server.redirect.end()) throw std::runtime_error(SSTR(line_number) + ": conflict with redirect");
					server.redirect[word] = line;
					// std::cout << word << ": " << server.redirect[word] << std::endl;
				} else throw std::runtime_error(SSTR(line_number) + ": " + word + ": Unrecognized server rule");
				break;

			case LOCATION_BLOCK:
				if (word.empty() and endl_char == '}') {
					if (location.root.size() == 0) throw std::runtime_error(SSTR(line_number) + ": No root specified");
					if (location.indexes.size() == 0) throw std::runtime_error(SSTR(line_number) + ": No index specified");
					if (server.locations.find(location.uri) != server.locations.end()) throw std::runtime_error(SSTR(line_number) + ": Duplicated location uri");
					server.locations[location.uri] = location;
					location.indexes.clear();
					location.methods.clear();
					blocklvl = SERVER_BLOCK;
				} else if (word == "root") {
					if (endl_char == '{') throw std::runtime_error(SSTR(line_number) + ": 'root' mustn't be a block");
					location.root = line.substr(line.find_first_not_of(" \t"));
					if (location.root.length() == 0) throw std::runtime_error(SSTR(line_number) + ": no root");
					if (location.root[location.root.length() - 1] == '/') location.root = location.root.substr(0, location.root.size() - 1);
					if (not isDir(location.root)) throw std::runtime_error(SSTR(line_number) + ": Unable to found root (" + location.root + ')');
				} else if (word == "index") {
					if (endl_char == '{') throw std::runtime_error(SSTR(line_number) + ": 'index' mustn't be a block");
					if (not getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": Invalid line");
					do {
						if (word.find('/') != std::string::npos) throw std::runtime_error(SSTR(line_number) + ": invalid filename");
						location.indexes.push_back(word);
					} while (getWord(line, word));
				} else if (word == "methods") {
					if (endl_char == '{') throw std::runtime_error(SSTR(line_number) + ": 'methods' mustn't be a block");
					if (not getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": Invalid line");
					do {
						if (word != "GET" && word != "POST" && word != "DELETE") throw std::runtime_error(SSTR(line_number) + ": unhandled or invalid method");
						location.methods.push_back(word);
					} while (getWord(line, word));
				} else if (word == "directory_listing") {
					if (endl_char == '{') throw std::runtime_error(SSTR(line_number) + ": 'directory listing' mustn't be a block");
					if (not getWord(line, word) or getWord(line, word)) throw std::runtime_error(SSTR(line_number) + ": invalid line");
					if (word != "on" and word != "off") throw std::runtime_error(SSTR(line_number) + ": 'directory_listing' argument must be 'on' or 'off'");
					location.directory_listing = (word == "on");
				} else throw std::runtime_error(SSTR(line_number) + ": " + word + ": Unrecognized location rule");
				break;

			case END:
				throw std::runtime_error(SSTR(line_number) + ": off-block line");

		}

		line.clear();

	}

	if (blocklvl != END) {
		throw std::runtime_error("Expected a closing brace at end of block");
	}

	file.close();

	for (std::vector<Server>::const_iterator server = _servers.begin(); server != _servers.end(); server++) {
		for (std::vector<Server>::const_iterator server2 = server + 1; server2 != _servers.end(); server2++) {
			if (server->servername == server2->servername) {
				throw std::runtime_error("conflicting with servers's name");
			}
		}
	}

}
