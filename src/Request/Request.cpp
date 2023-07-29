#include <Request/Request.hpp>
#include <iostream>

Request::Request(std::string & request, Server const * server) {

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

	std::string locationPath(_uri);

	std::cout << "looking for location: " << locationPath << std::endl;
	while (server->locations.find(locationPath) == server->locations.end()) {
		locationPath = locationPath.substr(0, locationPath.find_last_of('/'));
		if (locationPath.empty()) locationPath = "/";
		std::cout << "looking for location: " << locationPath << std::endl;
	}

	_location = &server->locations.at(locationPath);
	_uri.erase(0, locationPath.length());
	_target = _location->root + _uri;

}
