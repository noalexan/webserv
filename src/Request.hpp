#include <string>
#include <map>

class Request {

	private:

		std::string _method;
		std::string _uri;
		std::string _version;

		std::map<std::string, std::string> _headers;

	public:

		Request(std::string & request);

		std::string const & method() const { return _method; }
		std::string const & uri() const { return _uri; }
		std::string const & version() const { return _version; }

		std::map<std::string, std::string> const & headers() const { return _headers; }

};