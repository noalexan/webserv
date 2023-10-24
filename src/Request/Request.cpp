#include <Request/Request.hpp>
#include <sys/socket.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <utils/Colors.hpp>
#include <cstdlib>
#include <math.h>
#include <utils/utils.hpp>

#define BUFFER_SIZE 10240

static unsigned int atoi16(char const * str, size_t lenght) {
	if (lenght <= 0) return 0;
	if (('0' > *str or *str > '9') and ('a' > *str or *str > 'f'))
		throw std::runtime_error("invalid hexadecimal value");
	return (('0' <= *str and *str <= '9') ? *str - '0' : *str - 'a') * pow(16, lenght) + atoi16(str + 1, lenght - 1);
}

Request::Request(): _finished(false), _payload_too_large(false) {}

void Request::setFd(int const & fd) { _fd = fd; }

size_t Request::read(size_t const & max_body_size)  {

	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;

	bytes_read = recv(_fd, buffer, BUFFER_SIZE, MSG_DONTWAIT);

	if (bytes_read == -1) { // mais ta guuuuueeeuuuuuulleeee
		throw std::runtime_error("recv() failed"); // celui qui dit qui est
	} // 💣💣💣

	_request.append(buffer, bytes_read);

	size_t headerEndPos;

	if ((headerEndPos = _request.find("\r\n\r\n")) != std::string::npos) {

		size_t contentLengthPos;
		if ((contentLengthPos = _request.find("Content-Length")) != std::string::npos) {
			std::string sContentLength = _request.substr(contentLengthPos);
			sContentLength.erase(sContentLength.find("\r\n"));
			sContentLength = sContentLength.substr(sContentLength.find(": ") + 2);
			size_t contentLenght = atol(sContentLength.c_str());
			if (contentLenght > max_body_size) {
				_payload_too_large = true;
				_finished = true;
			} else if (contentLenght <= _request.length() - headerEndPos) {
				_request.erase(headerEndPos + contentLenght);
				_finished = true;
			}
		} else {
			_request.erase(headerEndPos);
			_finished = true;
		}

	}

	// std::cout << timestamp() << BHBLU << bytes_read << " bytes read" << std::endl;

	return bytes_read;

}

void Request::parse(Server const * server) {

	std::istringstream iss(_request);
	std::string line;

	std::getline(iss, line);

	_method = line.substr(0, line.find(' '));

	if (_method.length() == 0) {
		throw std::runtime_error("no method");
	}

	line.erase(0, line.find(' '));
	line.erase(0, line.find_first_not_of(' '));

	_uri = line.substr(0, line.find(' '));

	size_t pos;
	while ((pos = _uri.find('%')) != std::string::npos) {
		if (_uri.length() < pos + 3) {
			throw std::runtime_error("invalid hexadecimal value");
		}
		_uri.replace(pos, 3, SSTR((unsigned char) atoi16(_uri.c_str() + pos + 1, 2)));
	}

	if (_uri.length() == 0) {
		throw std::runtime_error("no uri");
	}

	if (_uri[0] != '/') {
		throw std::runtime_error("invalid request");
	}

	line.erase(0, line.find(' '));
	line.erase(0, line.find_first_not_of(' '));

	_version = line.substr(0, line.find_first_of("\r\n"));

	if (_version.length() == 0) {
		throw std::runtime_error("no version");
	}

	if (_version != "HTTP/1.1") {
		throw std::runtime_error("unhandled HTTP version");
	}

	while (std::getline(iss, line)) {

		if (line == "\r") break;

		std::string key = line.substr(0, line.find_first_of(':'));
		std::string value = line.substr(line.find_first_of(':') + 2, line.length());
		value = value.substr(0, value.size() - 1);
		_headers[key] = value;

	}

	if (_headers.find("Content-Length") != _headers.end()) {
		while (std::getline(iss, line)) {
			_body.append(line + '\n');
		}

	}

	std::string locationPath(_uri);

	std::cout << timestamp() << "looking for location: " << locationPath << std::endl;
	while (server->locations.find(locationPath) == server->locations.end()) {
		if (locationPath == "/") break;
		locationPath = locationPath.substr(0, locationPath.find_last_of('/'));
		if (locationPath.empty()) locationPath = "/";
		std::cout << timestamp() << "looking for location: " << locationPath << std::endl;
	}

	try {
		_location = (Location *) &server->locations.at(locationPath);
		_target = _location->root + '/' + _uri.substr(locationPath.length());
	} catch(std::exception &) {}

}
