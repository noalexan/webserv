#include <Request/Request.hpp>
#include <sys/socket.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <utils/Colors.hpp>

#define BUFFER_SIZE 1024

Request::Request(): _finished(false) {}

void Request::setFd(int const & fd) {
	_fd = fd;
}

void Request::read() {

	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;
	bytes_read = recv(_fd, buffer, BUFFER_SIZE, MSG_DONTWAIT);

	if (bytes_read == -1) {
		perror("recv");
		throw std::runtime_error("recv() failed");
	}

	_request.append(buffer, bytes_read);

	if (_request.find("Content-Length") != std::string::npos && 0) {
	}
	else if (_request.find("\r\n\r\n") != std::string::npos) {
		_finished = true;
		_request.erase(_request.find("\r\n\r\n"));
	}

}

bool Request::isFinished() const {
	return _finished;
}

void Request::parse(Server const * server) {

	std::istringstream iss(_request);
	std::string line;

	std::getline(iss, line);

	_method = line.substr(0, line.find(' '));

	line.erase(0, line.find(' '));
	line.erase(0, line.find_first_not_of(' '));

	_uri = line.substr(0, line.find(' '));

	while (_uri.find("%20") != std::string::npos) {
		_uri.replace(_uri.find("%20"), 3, " ");
	}

	line.erase(0, line.find(' '));
	line.erase(0, line.find_first_not_of(' '));

	_version = line.substr(0, line.find_first_of("\r\n"));

	while (std::getline(iss, line)) {

		if (line == "\r") break;

		std::string key = line.substr(0, line.find_first_of(':'));
		std::string value = line.substr(line.find_first_of(':') + 2, line.length());
		_headers[key] = value;

	}

	if (_headers.find("Content-Length") != _headers.end()) {
		_body.reserve(atoi(_headers["Content-Length"].c_str()));

		while (std::getline(iss, line)) {
			_body.append(line + '\n');
		}

	}

	std::string locationPath(_uri);

	std::cout << "looking for location: " << locationPath << std::endl;
	while (server->locations.find(locationPath) == server->locations.end()) {
		if (locationPath == "/") break;
		locationPath = locationPath.substr(0, locationPath.find_last_of('/'));
		if (locationPath.empty()) locationPath = "/";
		std::cout << "looking for location: " << locationPath << std::endl;
	}

	try {
		_location = (Location *) &server->locations.at(locationPath);
		_target = _location->root + _uri.substr(locationPath.length());
	} catch (std::exception &) {}

}
