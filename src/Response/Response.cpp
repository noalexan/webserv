#include <Response/Response.hpp>
#include <sys/socket.h>
#include <fstream>
#include <sys/stat.h>
#include <iostream>

#define BUFFER_SIZE 2

// static std::string readFile(std::string path) {
// 	std::ifstream file(path);
// 	if (not file.good()) throw std::runtime_error("unable to open '" + path + '\'');
// 	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
// 	file.close();
// 	return content;
// }

// static bool	isDirectory( std::string const & path ) {
// 	struct stat path_stat;
// 	stat(path.c_str(), &path_stat);
// 	return ( S_ISDIR(path_stat.st_mode) );
// }

// gros la go d'a cote elle abuse pour 3 gouttes elle a changé de place

// static bool isFile(std::string const & path) {
// 	struct stat path_stat;
// 	stat(path.c_str(), &path_stat);
// 	return ( S_ISREG(path_stat.st_mode) );
// }

Response::Response(): _response("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"), _finished(false) {
	_response += "readFile(\"public/images/ctardy.jpg\")\r\n";
	std::cout << _response << std::endl;
}

void Response::setFd(int fd) {
	_fd = fd;
}

void Response::write() {
	ssize_t bytes_sent = send(1, _response.c_str(), _response.length() % BUFFER_SIZE, 0);
	if (bytes_sent == -1) throw std::runtime_error("send() failed");
	if (bytes_sent != BUFFER_SIZE) _finished = true; // * https://www.youtube.com/watch?v=TDyG4YNUXcI
	_response.erase(bytes_sent); // ! https://www.youtube.com/watch?v=CVFkvTKfcqk
}

bool Response::isFinished() {
	return _finished;
}
