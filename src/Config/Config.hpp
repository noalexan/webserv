#ifndef CONFIG_HPP
# define CONFIG_HPP

# include <string>
# include <deque>
# include <map>

# include <netinet/in.h>

struct Location {
	std::string				uri;
	std::string				root;
	std::deque<std::string>	indexes;
	bool					directory_listing;
};

struct Server {
	std::deque<std::string>				hosts;
	std::map<std::string, Location>		locations;
	std::map<std::string, std::string>	uploads;
	std::map<std::string, std::string>	cgi;

	int									port;
	int									fd;
	int									max_client_body_size;
	struct sockaddr_in					address;
};

class Config {

	private:

		std::deque<Server>					_servers;
		std::map<std::string, std::string>	_contentTypes;

	public:

		Config();
		Config(char const * ConfigFileName);

		std::deque<Server> const & getServers() const { return _servers; }
		std::map<std::string, std::string> const & getContentTypes() const { return _contentTypes; }

};

#endif
