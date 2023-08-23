#include <Request/Request.hpp>
#include <iostream>
#include <sstream>

Request::Request(std::string & request, Server const & server) {

	// if (request.find("\r\n\r\n") == std::string::npos) {
	// 	throw std::runtime_error("Error: invalid request");
	// }

	std::istringstream iss(request);
	std::string line;

	std::getline(iss, line);

	_method = line.substr(0, line.find(' '));
	line.erase(0, _method.length());

	_uri = line.substr(line.find(' ') + 1, line.find_last_of(' ') - 1);
	line.erase(0, _uri.length());

	while (_uri.find("%20") != std::string::npos) {
		_uri.replace(_uri.find("%20"), 3, " ");
	}

	std::cout << '\'' << _uri << '\'' << std::endl;

	_version = line.substr(0, line.find_first_of("\r\n"));

	while (std::getline(iss, line)) {

		if (line == "\r") break;

		std::string key = line.substr(0, line.find_first_of(':'));
		std::string value = line.substr(line.find_first_of(':') + 2, line.length());
		_headers[key] = value;

	}

	while (std::getline(iss, line)) {
		if (line == "\r\n")
			continue;
		_body.append(line + '\n');
		if (_body.length() > server.max_client_body_size) {
			std::cout << _body.length() << std::endl;
			throw std::runtime_error("Body client size exceed limit");
		}
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
	while (server.locations.find(locationPath) == server.locations.end()) {
		locationPath = locationPath.substr(0, locationPath.find_last_of('/'));
		if (locationPath.empty()) locationPath = "/";
		std::cout << "looking for location: " << locationPath << std::endl;
	}

	_location = &server.locations.at(locationPath);
	_target = _location->root + _uri.substr(locationPath.length());

}
