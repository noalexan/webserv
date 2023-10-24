#include <iostream>
#include <fstream>
#include <sstream>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <dirent.h>

#include <algorithm>

#include <Response/Response.hpp>
#include <utils/Colors.hpp>
#include <utils/utils.hpp>

#define BUFFER_SIZE 10240

static std::string readFile(std::string const & path) {
	std::ifstream file(path.c_str());
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
	cookie += "id=" + SSTR(rand() % 100 + 1) + "; ";
	cookie += "HttpOnly;";
	cookie += "\r\n";

	return cookie;
}

Response::Response(): _finished(false) {}

void Response::setFd(int const & fd) { _fd = fd; }

void Response::handle(Request const & request, Server const * server, Config const & config, bool const & timeout){

	std::string const & uri = request.getUri();

	if (timeout) {
		std::cout << timestamp() << BYEL << "status 408" << CRESET << std::endl;
		responseMaker(server, "408", REQUEST_TIMEOUT);

	} else if (server->redirect.find(uri) != server->redirect.end()) {
		std::cout << timestamp() << BYEL << "status 301 (" << request.getMethod() << ')' << CRESET << std::endl;
		responseMaker(server, "301", MOVED, server->redirect.at(uri));

	} else if (request.getLocation() && std::find(request.getLocation()->methods.begin(), request.getLocation()->methods.end(), request.getMethod()) == request.getLocation()->methods.end() && (request.getMethod() != "POST" || server->uploads.find(uri) == server->uploads.end())) {
		std::cout << timestamp() << BYEL << "status 405 (" << request.getMethod() << ')' << CRESET << std::endl;
		responseMaker(server, "405", METHOD_NOT_ALLOWED);

	} else if (request.getMethod() == "GET") {

		std::string _target = request.getTarget();

		if (isDirectory(_target) or _target[_target.length() - 1] == '/') {
			if (_target[_target.length() - 1] != '/') _target += '/';

			for (std::vector<std::string>::const_iterator it = request.getLocation()->indexes.begin(); it != request.getLocation()->indexes.end(); it++) {
				if (isFile(_target + *it)) {
					_target += *it;
					break;
				}
			}

		}

		std::cout << timestamp() << "target: " << _target << std::endl;

		if (isDirectory(_target)) {

			if (request.getLocation()->directory_listing) {

				DIR *d;
				struct dirent *dir;

				d = opendir(_target.c_str());
				std::string uri = request.getUri();
				if (uri[uri.length() - 1] == '/') uri = uri.substr(0, uri.size() - 1);

				if (d) {

					std::string payload;

					while ((dir = readdir(d)) != NULL) {
						payload += "<a href=\"";

						if (dir->d_name[0] == '.' and dir->d_name[1] == 0) {
							payload += uri + "\">.</a><br/>";
						} else if (dir->d_name[0] == '.' and dir->d_name[1] == '.' and dir->d_name[2] == 0) {
							payload += uri.substr(0, uri.find_last_of('/')) + "/\">..</a><br/>";
						} else {
							payload += uri + '/' + dir->d_name + "\">" + dir->d_name + "</a><br/>";
						}

					}

					std::cout << timestamp() << BYEL << "status 200 (directory listing)" << CRESET << std::endl;
					_response += request.getVersion() + ' ' + OK + "\r\n";
					_response += "Content-Length: ";
					_response += SSTR(payload.length()) + "\r\n";
					_response += "Content-Type: text/html\r\n\r\n";
					_response += payload;

					closedir(d);
				}

			} else {
				std::cout << timestamp() << BYEL << "status 403 (" << _target << ')' << CRESET << std::endl;
				responseMaker(server, "403", FORBIDDEN);
			}

		} else if (isFile(_target)) {

			std::string filename = _target.substr(_target.find_last_of('/'));
			std::string extension = filename.substr(filename.find_last_of('.') + 1);

			if (isCGI(extension, server->cgi) and _target[_target.length() - 1] != '/') {

				std::cout << timestamp() << UCYN << "CGI " << extension << " ---> " << server->cgi.at(extension) << CRESET << std::endl;
				CGIMaker(server, extension, _target, OK);

			} else {

				std::cout << timestamp() << BYEL << "status 200 (" << _target << ')' << CRESET << std::endl;
				_response += request.getVersion() + ' ' + OK + "\r\n";

				std::string payload = readFile(_target);

				if (request.getHeaders().find("Cookie") == request.getHeaders().end()) {
					_response += bakeryCookies();
				}

				_response += "Content-Length: ";
				_response += SSTR(payload.length()) + "\r\n";

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
			std::cout << timestamp() << BYEL << "status 404 (" << _target << ')' << CRESET << std::endl;
			responseMaker(server, "404", NOT_FOUND);
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

				std::ofstream upload((server->uploads.at(uri) + '/' + filename).c_str());
				upload << content.substr(content.find("\r\n\r\n") + 4);

			} // ? Quand noah est en train de cook sérieusment, il ne faut pas le déranger
			std::cout << timestamp() << BYEL << "status 201 (" << uri << ')' << CRESET << std::endl;
			responseMaker(server, "201", CREATED);
		}

	// it's criminal
	} else if (request.getMethod() == "POST"
		&& isCGI((request.getTarget().substr(request.getTarget().find_last_of('/'))).substr(((request.getTarget().substr(request.getTarget().find_last_of('/')))).find_last_of('.') + 1), server->cgi)
		&& request.getTarget()[request.getTarget().length() - 1] != '/') {

		CGIMaker(server, (request.getTarget().substr(request.getTarget().find_last_of('/'))).substr(((request.getTarget().substr(request.getTarget().find_last_of('/')))).find_last_of('.') + 1), request.getTarget(), ACCEPTED);
	// it's criminal, but necessary

	} else if (request.getMethod() == "DELETE" && server->uploads.find(uri.substr(0, uri.find_last_of('/'))) != server->uploads.end()) {
		std::string target = server->uploads.at(uri.substr(0, uri.find_last_of('/'))) + uri.substr(uri.find_last_of('/'), uri.length()); 
		std::string folder = uri.substr(0, uri.find_last_of('/'));

		if (remove(target.c_str()) == -1) {
			std::cout << timestamp() << BYEL << "status 404 (" << target << ')' << CRESET << std::endl;
			responseMaker(server, "404", NOT_FOUND);

		} else {
			std::cout << timestamp() << BYEL << "status 204 (" << target << ")" << CRESET << std::endl;
			_response += request.getVersion() + ' ' + NO_CONTENT + "\r\n\r\n";
		}

	} else {
		std::cout << timestamp() << BYEL << "status 403" << CRESET << std::endl;
		responseMaker(server, "403", FORBIDDEN);
	}
}

void Response::responseMaker(Server const * const server, std::string const & statusCode, std::string const & statusHeader) {
	std::string payload  = (server->pages.find(statusCode) != server->pages.end()) ? readFile(server->pages.at(statusCode)) : statusHeader + "\r\n";

	_response += std::string("HTTP/1.1 ") + statusHeader + "\r\n";
	_response += std::string("Content-Length: ") + SSTR(payload.length()) + "\r\n";
	_response += "Content-Type: text/html\r\n\r\n";
	_response += payload;
}

void Response::responseMaker(Server const * const server, std::string const & statusCode, std::string const & statusHeader, std::string const & redirection) {
	std::string payload  = (server->pages.find(statusCode) != server->pages.end()) ? readFile(server->pages.at(statusCode)) : statusHeader + "\r\n";

	_response += std::string("HTTP/1.1 ") + statusHeader + "\r\n";
	_response += std::string("Content-Length: ") + SSTR(payload.length()) + "\r\n";
	_response += "Location: " + redirection + "\r\n";
	_response += "Content-Type: text/html\r\n\r\n";
	_response += payload;
}

void Response::CGIMaker(Server const * server, std::string const & extension, std::string const & target, std::string const & statusHeader) {

	int	fd[2];
	if (pipe(fd) < 0) throw std::runtime_error("Error pipe()");

	int pid = fork();
	if (pid == 0) {

		try {

			if (close(fd[0]) == -1)               throw std::runtime_error("Error closing pipe fd[0] [SON]");
			if (dup2(fd[1], STDOUT_FILENO) == -1) throw std::runtime_error("Error redirecting stdout [SON]");
			if (close(fd[1]) == -1)               throw std::runtime_error("Error closing pipe fd[1] [SON]");

			char *args[] = { (char *)server->cgi.at(extension).c_str(), (char *)target.c_str(), NULL };

			alarm(5);
			if (execve( server->cgi.at(extension).c_str(), args, server->env ) == -1) {
				throw std::runtime_error("Error execve");
			}

		} catch (std::exception & e) {
			std::cerr << e.what() << std::endl;
			exit(EXIT_FAILURE);
		}

	} else {

		if (close(fd[1]) == -1) throw std::runtime_error("Error closing pipe fd[1] [PARENT]");

		char buffer[BUFFER_SIZE];
		ssize_t bytes_read;
		std::string cgi_output;

		while ((bytes_read = read(fd[0], buffer, BUFFER_SIZE)) > 0) {
			cgi_output.append(buffer, bytes_read);
		}

		if (close(fd[0]) == -1) throw std::runtime_error("Error closing pipe fd[0] [PARENT]");

		int status;

		waitpid(pid, &status, 0);
		if (WIFEXITED(status) and WEXITSTATUS(status) == 0) {

			std::cout << timestamp() << BYEL << "status " << statusHeader.substr(0, 3) << " (cgi)" << CRESET << std::endl;

			_response += std::string("HTTP/1.1 ") + statusHeader + "\r\n";
			_response += "Content-Length: ";
			_response += SSTR(cgi_output.length()) + "\r\n";
			_response += "Content-Type: text/html\r\n\r\n";
			_response += cgi_output;

		} else if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) {
			kill(pid, SIGKILL);
			std::cout << timestamp() << BYEL << "status 504 (" << target << ')' << CRESET << std::endl;
			responseMaker(server, "504", GATEWAY_TIMEOUT);

		} else {
			std::cout << timestamp() << BYEL << "status 500 (" << target << ')' << CRESET << std::endl;
			responseMaker(server, "500", INTERNAL_ERROR);
		}

	}
}

void Response::write() {
	ssize_t bytes_sent = send(_fd, _response.c_str(), ((BUFFER_SIZE <= _response.length()) ? BUFFER_SIZE : (_response.length() % BUFFER_SIZE)), 0);
	if (bytes_sent == -1) {
		throw std::runtime_error("send() failed");
	}
	_response.erase(0, bytes_sent);
	if (_response.empty()) _finished = true;
	// std::cout << timestamp() << BHBLU << bytes_sent << " bytes sent" << std::endl;
}

bool const & Response::isFinished() const {
	return _finished;
}
