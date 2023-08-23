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
		std::string _body;

		Location const * _location;

		std::string _target;

		std::map<std::string, std::string> _headers;
		std::map<std::string, std::string> _params;

	public:

		Request(std::string & request, Server const & server);

		std::string const & getMethod() const { return _method; }
		std::string const & getUri() const { return _uri; }
		std::string const & getVersion() const { return _version; }
		std::string const & getBody() const { return _body; }
		Location const * getLocation() const { return _location; }
		std::string const & getTarget() const { return _target; }
		std::map<std::string, std::string> const & getHeaders() const { return _headers; }
		std::map<std::string, std::string> const & getParams() const { return _params; }

};

#endif
