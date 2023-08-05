#include <Request/Request.hpp>
#include <iostream>

Request::Request(std::string & request, Server const * server) {

	if (request.find("\r\n\r\n") == std::string::npos) {
		std::cerr << "Error: request not finished" << std::endl;
		return;
	}

	_method = request.substr(0, request.find(' '));
	request.erase(0, request.find(' ') + 1);

	if (_method == "POST") std::cout << std::endl << std::endl;

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

			std::cout << "param: " << key << "=" << value << std::endl;

		}

	}

	if (_method == "POST") {
		if (_headers.find("Content-Type") == _headers.end()) {}
		else if (_headers.at("Content-Type") == "application/x-www-form-urlencoded") {

			std::string parameters = request.substr(request.find("\r\n\r\n") + 3, request.length());

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
				std::cout << "param: " << key << "=" << value << std::endl;
			}
		}
		else { throw std::runtime_error("Error: unsupported Content-Type"); }
	}


	std::string locationPath(_uri);

	std::cout << "looking for location: " << locationPath << std::endl;
	while (server->locations.find(locationPath) == server->locations.end()) {
		locationPath = locationPath.substr(0, locationPath.find_last_of('/'));
		if (locationPath.empty()) locationPath = "/";
		std::cout << "looking for location: " << locationPath << std::endl;
	}
	std::cout << std::endl;

	_location = &server->locations.at(locationPath);
	_uri.erase(0, locationPath.length());
	_target = _location->root + _uri;

}
