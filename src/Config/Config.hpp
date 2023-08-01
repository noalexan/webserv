#ifndef CONFIG_HPP
# define CONFIG_HPP

# include <string>
# include <deque>
# include <map>

# include <netinet/in.h>

# define MAX_EVENTS 100

struct Location {
	std::string				root;
	std::deque<std::string>	indexes;
	bool					directoryListing;
};

struct Server {
	std::deque<std::string>			hosts;
	std::map<std::string, Location>	locations;

	int								port;
	int								fd;
	struct sockaddr_in				address;
};

class Config {

	private:

		std::deque<Server>					_servers;
		std::map<std::string, std::string>	_contentTypes;
		std::map<std::string, std::string>	_environment;

	public:

		Config();
		Config(char const * ConfigFileName, char const * const * env);

		std::deque<Server>			& getServers( void ) { return _servers; }
		std::deque<Server> const	& getServers( void ) const { return _servers; }

		std::map<std::string, std::string> const	& getContentTypes() const { return _contentTypes; }
		std::map<std::string, std::string> const	& getEnvironment() const { return _environment; }

};

#endif
