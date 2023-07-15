#ifndef WEBSERVCONFIG_HPP
# define WEBSERVCONFIG_HPP

# include <string>
# include <deque>
# include <map>

# include <netinet/in.h>

struct Location {
	std::string				root;
	std::deque<std::string>	indexes;
};

struct Server {
	std::deque<std::string>			hosts;
	std::map<std::string, Location>	locations;

	int								port;
	int								fd;
	struct sockaddr_in				address;
};

class WebservConfig {

	private:

		std::deque<Server> _servers;

	public:

		WebservConfig();
		WebservConfig(char const * ConfigFileName);

		std::deque<Server>			& servers() { return _servers; }
		std::deque<Server> const	& servers() const { return _servers; }

};

#endif
