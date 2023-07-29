#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <string>
# include <map>
# include <Config/Config.hpp>

class Request {

	private:

		std::string _method;
		std::string _uri;
		std::string _version;

		Location const * _location;

		std::string _target;

		std::map<std::string, std::string> _headers;

	public:

		Request(std::string & request, Server const * server);

		std::string const & getMethod() const { return _method; }
		std::string const & getUri() const { return _uri; }
		std::string const & getVersion() const { return _version; }
		Location const * getLocation() const { return _location; }
		std::string const & getTarget() const { return _target; }
		std::map<std::string, std::string> const & headers() const { return _headers; }

};

#endif
