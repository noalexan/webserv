#ifndef WEBSERVCONFIG_HPP
# define WEBSERVCONFIG_HPP

# include <string>
# include <deque>
# include <map>

struct Location {
	std::string root;
	std::deque<std::string> indexes;
};

struct Server {
	std::deque<std::string> hosts;
	int port;
	std::map<std::string, Location> locations;
};

class WebservConfig {

	private:
		std::deque<Server> _servers;

	public:
		WebservConfig(char const * ConfigFileName);

		std::deque<Server> & servers() { return _servers; }
		std::deque<Server> const & servers() const { return _servers; }

};

#endif
