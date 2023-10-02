#include <unistd.h>
#include <stdio.h>
#include <Response/Response.hpp>
#include <sys/socket.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <dirent.h>
#include <utils/Colors.hpp>

#define BUFFER_SIZE 10240

static std::string readFile(std::string const & path) {
	std::ifstream file(path);
	if (not file.good()) throw std::runtime_error("unable to open '" + path + '\'');
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return content;
}

static bool	isDirectory( std::string const & path ) {
	struct stat path_stat;
	stat(path.c_str(), &path_stat);
	return ( S_ISDIR(path_stat.st_mode) );
}

static bool isFile(std::string const & path) {
	struct stat path_stat;
	stat(path.c_str(), &path_stat);
	return ( S_ISREG(path_stat.st_mode) );
}

static bool isCGI( std::string const & extension, std::map<std::string, std::string> const & cgi ) {
	return cgi.find(extension) != cgi.end();
}

static std::string const bakeryCookies() {

	std::string cookie;

	cookie += "Set-Cookie: ";
	cookie += "id=" + std::to_string(rand() % 100 + 1) + "; ";
	cookie += "Secure; HttpOnly;";
	cookie += "\r\n";

	return cookie;
}

Response::Response(): _finished(false) {}

void Response::setFd(int const & fd) {
	_fd = fd;
}

void Response::handle(Request const & request, Server const * server, Config const & config, bool const & timeout){

	std::string const & uri = request.getUri();

	if (timeout) {
		std::string payload = (server->pages.find("408") != server->pages.end()) ? readFile(server->pages.at("408")) : "Request Timeout\r\n";
		std::cout << BYEL << "status 408" << CRESET << std::endl;

		_response += std::string("HTTP/1.1 ") + REQUEST_TIMEOUT + "\r\n";
		_response += std::string("Content-Length: ") + std::to_string(payload.length()) + "\r\n";
		_response += "Content-Type: text/html\r\n\r\n";
		_response += payload;

	} else if (request.getLocation() && std::find(request.getLocation()->methods.begin(), request.getLocation()->methods.end(), request.getMethod()) == request.getLocation()->methods.end() && (request.getMethod() != "POST" || server->uploads.find(uri) == server->uploads.end())) {
		std::cout << BYEL << "status 405 (" << request.getMethod() << ')' << CRESET << std::endl;
		std::string payload = (server->pages.find("405") != server->pages.end()) ? readFile(server->pages.at("405")) : "Method Not Allowed\r\n";

		_response += std::string("HTTP/1.1 ") + METHOD_NOT_ALLOWED + "\r\n";
		_response += std::string("Content-Length: ") + std::to_string(payload.length()) + "\r\n";
		_response += "Content-Type: text/html\r\n\r\n";
		_response += payload;

	} else if (request.getMethod() == "GET") {

		std::string _target = request.getTarget();

		if (isDirectory(_target) or _target[_target.length() - 1] == '/') {
			if (_target[_target.length() - 1] != '/') _target += '/';

			for (std::deque<std::string>::const_iterator it = request.getLocation()->indexes.begin(); it != request.getLocation()->indexes.end(); it++) {
				if (isFile(_target + *it)) {
					_target += *it;
					break;
				}
			}

		}

		std::cout << "target: " << _target << std::endl;

		if (isDirectory(_target)) {

			if (request.getLocation()->directory_listing) {

				DIR *d;
				struct dirent *dir;
				d = opendir(_target.c_str());
				std::string uri = request.getUri();
				if (uri[uri.length() - 1] == '/') uri.pop_back();
				if (d) {

					std::cout << BYEL << "status 200 (directory listing)" << CRESET << std::endl;

					std::string payload;

					while ((dir = readdir(d)) != nullptr) {
						payload += "<a href=\"";

						if (dir->d_name[0] == '.' and dir->d_name[1] == 0) {
							payload += uri + "\">.</a><br/>";
						} else if (dir->d_name[0] == '.' and dir->d_name[1] == '.' and dir->d_name[2] == 0) {
							payload += uri.substr(0, uri.find_last_of('/')) + "/\">..</a><br/>";
						} else {
							payload += uri + '/' + dir->d_name + "\">" + dir->d_name + "</a><br/>";
						}

					}

					_response += request.getVersion() + ' ' + OK + "\r\n";
					_response += "Content-Length: ";
					_response += std::to_string(payload.length()) + "\r\n";
					_response += "Content-Type: text/html\r\n\r\n";
					_response += payload;

				}

				closedir(d);

			} else {

				std::string payload = server->pages.find("403") != server->pages.end() ? readFile(server->pages.at("403")) : "Forbidden\r\n";
				std::cout << BYEL << "status 403 (" << _target << ')' << CRESET << std::endl;

				_response += request.getVersion() + ' ' + FORBIDDEN + "\r\n";
				_response += "Content-Length: ";
				_response += std::to_string(payload.length()) + "\r\n";
				_response += "Content-Type: text/html\r\n\r\n";
				_response += payload;

			}

		} else if (isFile(_target)) {

			std::string filename = _target.substr(_target.find_last_of('/'));
			std::string extension = filename.substr(filename.find_last_of('.') + 1);

			if (isCGI(extension, server->cgi) and _target[_target.length() - 1] != '/') {

				std::cout << UCYN << "CGI " << extension << " ---> " << server->cgi.at(extension) << CRESET << std::endl;

				int	fd[2];
				if (pipe(fd) < 0) {
					throw std::runtime_error("Error pipe()");
				}

				int pid = fork();
				if (pid == 0) {

					try {

						if (close(fd[0]) == -1)               throw std::runtime_error("Error closing pipe fd[0] [SON]");
						if (dup2(fd[1], STDOUT_FILENO) == -1) throw std::runtime_error("Error redirecting stdout [SON]");
						if (close(fd[1]) == -1)               throw std::runtime_error("Error closing pipe fd[1] [SON]");

						char *args[] = { (char *)server->cgi.at(extension).c_str(), (char *)_target.c_str(), nullptr };

						if (execve( server->cgi.at(extension).c_str(), args, server->env ) == -1) {
							throw std::runtime_error("Error execve");
						}

					} catch (std::exception & e) {
						std::cerr << e.what() << std::endl;
						exit(EXIT_FAILURE);
					}

				} else {

					if (close(fd[1]) == -1) {
						throw std::runtime_error("Error closing pipe fd[1] [PARENT]");
					}

					char buffer[BUFFER_SIZE];
					ssize_t bytes_read;
					std::string cgi_output;

					while ((bytes_read = read(fd[0], buffer, BUFFER_SIZE)) > 0) {
						cgi_output.append(buffer, bytes_read);
					}

					if (close(fd[0]) == -1) {
						throw std::runtime_error("Error closing pipe fd[0] [PARENT]");
					}

					int status;
					waitpid(pid, &status, 0);

					if (WIFEXITED(status) and WEXITSTATUS(status) == 0) {

						std::cout << BYEL << "status 200 (cgi)" << CRESET << std::endl;

						_response += request.getVersion() + ' ' + OK + "\r\n";
						_response += "Content-Length: ";
						_response += std::to_string(cgi_output.length()) + "\r\n";
						_response += "Content-Type: text/html\r\n\r\n";
						_response += cgi_output;

					} else {
						throw std::runtime_error("Error fork()");
					}

				}
			} else {

				std::cout << BYEL << "status 200 (" << _target << ')' << CRESET << std::endl;

				std::string payload = readFile(_target);

				_response += request.getVersion() + ' ' + OK + "\r\n";

				if (request.getHeaders().find("Cookie") == request.getHeaders().end())
					_response += bakeryCookies();

				_response += "Content-Length: ";
				_response += std::to_string(payload.length()) + "\r\n";

				std::map<std::string, std::string> const & contentTypes = config.getContentTypes();
				if (contentTypes.find(extension) != contentTypes.end()) {
					_response += "Content-Type: ";
					_response += contentTypes.at(extension) + "\r\n";
				} else {
					std::cerr << "unhandled extension: '" << extension << '\'' << std::endl;
				}
				_response += "\r\n";
				_response += payload;

			}
		} else {

			std::string payload  = (server->pages.find("404") != server->pages.end()) ? readFile(server->pages.at("404")) : "Not Found\r\n";
			std::cout << BYEL << "status 404 (" << _target << ')' << CRESET << std::endl;

			_response += request.getVersion() + ' ' + NOT_FOUND + "\r\n";
			_response += "Content-Length: ";
			_response += std::to_string(payload.length()) + "\r\n";
			_response += "Content-Type: text/html\r\n\r\n";
			_response += payload;

		}

	} else if (request.getMethod() == "POST" && server->uploads.find(uri) != server->uploads.end()) {
		if (request.getHeaders().find("Content-Type") != request.getHeaders().end()) {

			std::string body = request.getBody();
			std::string contentType = request.getHeaders().at("Content-Type");
			std::string boundary = std::string("--") + contentType.substr(contentType.find("boundary=") + 9);

			body.erase(0, body.find(boundary) + boundary.length() + 2);
			while (body.length()) {

				std::string content = body.substr(0, body.find(boundary) - 2);
				body.erase(0, body.find(boundary) + boundary.length() + 2);

				std::string filename = content.substr(content.find("filename=\"") + 10);
				filename.erase(filename.find('"'));

				std::ofstream upload(server->uploads.at(uri) + '/' + filename);
				upload << content.substr(content.find("\r\n\r\n") + 4);

			} // ? Quand noah est en train de cook sérieusment, il ne faut pas le déranger

			std::string payload  = (server->pages.find("201") != server->pages.end()) ? readFile(server->pages.at("201")) : "Created\r\n";
			std::cout << BYEL << "status 201 (" << uri << ')' << CRESET << std::endl;

			_response += request.getVersion() + ' ' + CREATED + "\r\n";
			_response += "Content-Length: ";
			_response += std::to_string(payload.length()) + "\r\n";
			_response += "Content-Type: text/html\r\n\r\n";
			_response += payload;
		}

	} else if (request.getMethod() == "DELETE" && server->uploads.find(uri.substr(0, uri.find_last_of('/'))) != server->uploads.end()) {
		std::string target = server->uploads.at(uri.substr(0, uri.find_last_of('/'))) + uri.substr(uri.find_last_of('/'), uri.length()); 
		std::string folder = uri.substr(0, uri.find_last_of('/'));

		if (remove(target.c_str()) == -1) {
			std::string payload  = (server->pages.find("404") != server->pages.end()) ? readFile(server->pages.at("404")) : "Not Found\r\n";
			std::cout << BYEL << "status 404 (" << target << ')' << CRESET << std::endl;

			_response += request.getVersion() + ' ' + NOT_FOUND + "\r\n";
			_response += "Content-Length: ";
			_response += std::to_string(payload.length()) + "\r\n";
			_response += "Content-Type: text/html\r\n\r\n";
			_response += payload;

		} else {
			std::cout << BYEL << "status 204 (" << target << ")" << CRESET << std::endl;
			_response += request.getVersion() + ' ' + NO_CONTENT + "\r\n\r\n";
		}

	} else {
		std::string payload  = (server->pages.find("403") != server->pages.end()) ? readFile(server->pages.at("403")) : "Forbidden\r\n";
		std::cout << BYEL << "status 403" << CRESET << std::endl;

		_response += request.getVersion() + ' ' + FORBIDDEN + "\r\n";
		_response += "Content-Length: ";
		_response += std::to_string(payload.length()) + "\r\n";
		_response += "Content-Type: text/html\r\n\r\n";
		_response += payload;
	}
}

void Response::payloadTooLarge(Server const * const server) {
	std::cout << BYEL << "status 413" << CRESET << std::endl;
	std::string payload  = (server->pages.find("413") != server->pages.end()) ? readFile(server->pages.at("413")) : "Payload Too Large\r\n";

	_response += std::string("HTTP/1.1 ") + PAYLOAD_TOO_LARGE + "\r\n";
	_response += std::string("Content-Length: ") + std::to_string(payload.length()) + "\r\n";
	_response += "Content-Type: text/html\r\n\r\n";
	_response += payload;
}

void Response::write() {
	ssize_t bytes_sent = send(_fd, _response.c_str(), ((BUFFER_SIZE <= _response.length()) ? BUFFER_SIZE : (_response.length() % BUFFER_SIZE)), 0);
	if (bytes_sent == -1) throw std::runtime_error("send() failed");
	_response.erase(0, bytes_sent);
	if (_response.empty()) _finished = true;
	// std::cout << BHBLU << bytes_sent << " bytes sent" << std::endl;
}

bool const & Response::isFinished() const {
	return _finished;
}
