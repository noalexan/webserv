#include <Request/Request.hpp>
#include <sys/socket.h>
#include <iostream>
#include <sstream>
#include <unistd.h>

#define BUFFER_SIZE 256

Request::Request(): _finished(false) {}

void Request::setFd(int fd) {
	_fd = fd;
}

void Request::read() {
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;
	bytes_read = recv(_fd, buffer, BUFFER_SIZE, MSG_DONTWAIT);
	if (bytes_read == -1) {
		perror("recv"); // la go elle me fait rire a faire ses meeting a starbucks
		throw std::runtime_error("recv() failed");
	}
	if (bytes_read != BUFFER_SIZE) _finished = true;
	_request.append(buffer, bytes_read);
}

bool Request::isFinished() {
	return _finished;
}

void Request::print() {
	std::cout << _request << std::endl;
}

void Request::parse() {

	std::istringstream iss(_request);
	std::string line;

	std::getline(iss, line); // a cote il essaie de pecho la go breeeff

	_method = line.substr(0, line.find(' '));

	line.erase(0, line.find(' '));
	line.erase(0, line.find_first_not_of(' '));

	_uri = line.substr(0, line.find(' '));

	line.erase(0, line.find(' '));
	line.erase(0, line.find_first_not_of(' '));

	_version = line.substr(0, line.find_first_of("\r\n"));

	while (std::getline(iss, line)) {

		if (line == "\r") break;

		std::string key = line.substr(0, line.find_first_of(':'));
		std::string value = line.substr(line.find_first_of(':') + 2, line.length());
		_headers[key] = value;

	}

	std::cout << "CA MARCHE OUUUUUUU" << std::endl; // ** https://www.youtube.com/watch?v=PYkA4WpR5Ic&pp=ygUgZ2VvcmdlIG1vdXN0YWtpIGxlcyBlYXV4IGRlIG1hcnM%3D

	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); it++) {
		std::cout << it->first << ": " << it->second << "\r\n";
	}

}

// ? https://www.youtube.com/watch?v=VqsU_4KOEA8
// tu peux relancer un terminal ????????????????????????????????????????????????????????????????????????????????????????????????????????
