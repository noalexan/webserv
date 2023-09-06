#include <Request/Request.hpp>
#include <sys/socket.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <utils/Colors.hpp>

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
		perror("recv");
		throw std::runtime_error("recv() failed");
	}

	/*
	
		* tu check si y a un content length
		 - si y a:
		 	* tu lis jusqua tant que la taille du body soit egale
		 - sinon
		 	* tu check juste le \r\n\r\n;
	
	*/

	// size_t contentLengthPos = _request.find_first_of("Content-Length: ");
	// if ((contentLengthPos == std::string::npos && _request.find("\r\n\r\n") != std::string::npos) /* or read until body length = contentLength */ ) _finished = true;
	// std::string contentLength = (contentLengthPos != std::string::npos) ? _request.substr(contentLengthPos) : "";
	// contentLength.erase(contentLength.find_first_of("\r\n"));

	_request.append(buffer, bytes_read);

	if (_request.find("Content-Length") != std::string::npos && 0) {
/*
		size_t	contentLength = atoi( _request.substr(_request.find("Content-Length: ") + 16, _request.find("\r\n")).c_str() );
		std::string boundary = _request.substr(_request.find("Boundary"));
		// std::cout << "length:		"<< BBLK  << _request.substr(_request.find("Content-Length: ") + 16, 1 - _request.find("\r\n")) << CRESET <<std::endl;
		std::cout << "length:		"<< BCYN  << contentLength << CRESET <<std::endl;
		std::cout << "boundary:	"<< BYEL  << boundary.substr(boundary.find("Boundary") + 8, boundary.find("\r\n")) << CRESET <<std::endl;
*/
	}
	else if (_request.find("\r\n\r\n") != std::string::npos) {
		_finished = true;
		_request.erase(_request.find("\r\n\r\n"));
	}
}

bool Request::isFinished() {
	return _finished;
}

void Request::parse(Server const * server) {

	std::cout << BCYN << _request << CRESET << std::endl;

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

	// std::cout << BRED << "line: " << line << "|end of line" << CRESET << std::endl;

	// if (line == "\r") {
	// 	while (std::getline(iss, line)) {
	// 		_body += line;
	// 		std::cout << BRED << "body: " << _body << CRESET << std::endl;
	// 	}
	// }

	if (_headers.find("Content-Length") != _headers.end())
		_body.reserve( atoi(_headers["Content-Length"].c_str()) );
	while (std::getline(iss, line)) {
		// std::cout << BHBLU << "the line: " << line << CRESET << RED << "\\r\\n" << CRESET << std::endl;
		if (line == "\r\n")
			continue ;
		_body.append(line + '\n');
	}
	// std::cout << BBLU << _body << CRESET << std::endl;

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
