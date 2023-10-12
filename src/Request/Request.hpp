#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <Config/Config.hpp>

#include <string>
#include <map>

class Request {

	protected:
		bool        _finished;
		bool        _payload_too_large;
		std::string _request;
		int         _fd;

		std::string _method;
		std::string _uri;
		std::string _version;
		std::map<std::string, std::string> _params;
		std::map<std::string, std::string> _headers;
		std::string _body;
		std::string _target;
		Location *  _location;

	public:
		Request();

		void   setFd(int const & fd);
		size_t read(size_t const & max_body_size);
		void   parse(Server const * const);

		bool const & isFinished() const { return _finished; }
		bool const & isTooLarge() const { return _payload_too_large; }

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
