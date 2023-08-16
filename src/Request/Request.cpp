#include <Request/Request.hpp>
#include <iostream>
#include <sstream>

Request::Request(std::string & request, Server const * server) {

	if (request.find("\r\n\r\n") == std::string::npos) {
		throw std::runtime_error("Error: invalid request");
	}

	std::istringstream iss(request);
	std::string line;

	std::getline(iss, line);

	_method = line.substr(0, line.find(' '));

	line.erase(0, line.find(' '));
	line.erase(0, line.find_first_not_of(' '));

	_uri = line.substr(0, line.find(' '));

	line.erase(0, line.find(' '));
	line.erase(0, line.find_first_not_of(' '));

	_version = line;

	while (std::getline(iss, line)) {

		if (line == "\r") break;

		std::string key = line.substr(0, line.find_first_of(':'));
		std::string value = line.substr(line.find_first_of(':') + 2, line.length());
		_headers[key] = value;

	}

	while (std::getline(iss, line)) {
		_body.append(line);
	}

	if (_uri.find('?') != std::string::npos) {

		std::string parameters = _uri.substr(_uri.find('?') + 1, _uri.length());
		_uri.erase(_uri.find('?'), _uri.length());

		while (parameters.length()) {

			std::string key = parameters.substr(0, parameters.find('='));

			if (parameters.find('=') != std::string::npos) {
				parameters.erase(0, parameters.find('=') + 1);
			} else {
				parameters.erase(0, parameters.length());
			}

			std::string value = parameters.substr(0, parameters.find('&'));

			if (parameters.find('&') != std::string::npos) {
				parameters.erase(0, parameters.find('&') + 1);
			} else {
				parameters.erase(0, parameters.length());
			}

			_params[key] = value;

		}

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
