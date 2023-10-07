#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

#include <netinet/in.h>

struct Address {
	int                port;
	int                fd;
	struct sockaddr_in address;
};

struct Location {
	std::string              uri;
	std::string              root;
	std::vector<std::string> indexes;
	std::vector<std::string> methods;
	bool                     directory_listing;
};

struct Server {
	std::vector<Address>               addresses;
	std::map<std::string, Location>    locations;
	std::map<std::string, std::string> redirect;
	std::map<std::string, std::string> uploads;
	std::map<std::string, std::string> pages;
	std::map<std::string, std::string> cgi;

	std::string                        servername;
	unsigned long                      max_client_body_size;
	char                               **env;

};

class Config {

	private:
		std::vector<Server>                _servers;
		std::map<std::string, std::string> _contentTypes;

	public:
		Config();

		void load(char const * ConfigFileName, char **env);

		std::vector<Server> const & getServers() const { return _servers; }
		std::map<std::string, std::string> const & getContentTypes() const { return _contentTypes; }

};

#endif
