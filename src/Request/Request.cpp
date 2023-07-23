#include <Request/Request.hpp>
#include <iostream>

Request::Request(std::string & request) {

	if (request.find("\r\n\r\n") == std::string::npos) {
		std::cerr << "Error: invalid request" << std::endl;
		return;
	}

	_method = request.substr(0, request.find(' '));
	request.erase(0, request.find(' ') + 1);

	_uri = request.substr(0, request.find(' '));
	request.erase(0, request.find(' ') + 1);

	_version = request.substr(0, request.find("\r\n"));
	request.erase(0, request.find("\r\n") + 2);

	std::string line;
	while ((line = request.substr(0, request.find("\r\n"))).length()) {

		request.erase(0, request.find("\r\n") + 2);

		std::string key = line.substr(0, line.find(": "));
		line.erase(0, line.find(": ") + 2);

		_headers[key] = line;

	}

}
