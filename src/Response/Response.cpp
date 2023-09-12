#include <unistd.h>
#include <Response/Response.hpp>
#include <sys/socket.h>
#include <fstream>
#include <sys/stat.h>
#include <iostream>
#include <dirent.h>
#include <utils/Colors.hpp>

#define BUFFER_SIZE 40960

static std::string readFile(std::string path) {
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

static bool isCGI( std::string const & extension, std::map<std::string, std::string> cgi ) {
	if (cgi.find(extension) == cgi.end())
		return (false);
	return (true);
}

static bool isGlobalCGI( std::string const & extension ) {
	std::string	cgi[] = { "cpp", "c", "js", "ts", "java", "py", "php", "bf", "cs", "go", "rs", "bas", "sh" };
	if (cgi->find(extension) == std::string::npos)
		return (false);
	return (true);
}

Response::Response(): _finished(false) {}

void Response::setFd(int fd) {
	_fd = fd;
}

void Response::handle(Request const & request, Server const * server) {

	std::cout << request.getUri() << std::endl;
	std::cout << request.getMethod() << std::endl;

	if (server->uploads.find(request.getUri()) != server->uploads.end()) {

		if (request.getMethod() == "POST") {
			// bodyParser(request, server);
			std::cout << "upload request" << std::endl;
			// for (std::map<std::string, std::string>::const_iterator it = request.getHeaders().begin(); it != request.getHeaders().end(); it++)
			// 	std::cout << BGRN << it->first << ": " << it->second << CRESET << std::endl;
			// std::cout << BYEL << request.getBody() << CRESET << std::endl;
		} else if (request.getMethod() == "DELETE") {
			std::cout << "delete request" << std::endl;
		}

	}

	if (request.getMethod() == "GET") {

		std::string _target = request.getTarget();

		if (isDirectory(_target) || _target[_target.length() - 1] == '/') {
			if (_target[_target.length() - 1] != '/') _target += '/';

			for (std::deque<std::string>::const_iterator it = request.getLocation()->indexes.begin(); it != request.getLocation()->indexes.end(); it++) {
				if (isFile(_target + *it)) {
					_target += *it;
					break;
				}
			}
		}
	
		else if (isCGI(_target.substr(_target.find_last_of(".") + 1, _target.length()), server->cgi)
			&& isGlobalCGI(_target.substr(_target.find_last_of(".") + 1, _target.length()))
			&& _target[_target.length() - 1] != '/') {
			std::string extension = _target.substr(_target.find_last_of(".") + 1, _target.length());
			std::cout << BRED << _target << CRESET << std::endl;
			int	fd[2];
			if (pipe(fd) < 0)
				throw std::runtime_error("Error pipe()");
			int pid = fork();
			if (!pid) {

				close(fd[0]);
				if (dup2(fd[1], STDOUT_FILENO) == -1) {
					throw std::runtime_error("Error redirecting stdout");
				}
				close(fd[1]);

				char *args[] = { (char *)server->cgi.at(extension).c_str(), (char *)_target.c_str(), NULL };
				execve( server->cgi.at(extension).c_str(), args, server->env );

				extension.clear();
				exit(EXIT_FAILURE);
			} else {
				close(fd[1]);

				char buffer[BUFFER_SIZE];
				ssize_t bytes_read;
				std::string cgi_output;

				while ((bytes_read = read(fd[0], buffer, BUFFER_SIZE)) > 0) {
					cgi_output.append(buffer, bytes_read);
				}

				close(fd[0]);

				int status;
				waitpid(pid, &status, 0);

				if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
					_response += request.getVersion() + ' ' + OK + "\r\n";
					_response += "Server: webserv\r\n";
					_response += "Connection: close\r\n";
					_response += "Content-Type: text/html\r\n\r\n";
					_response += cgi_output;

					_finished = true;
				} else
					throw std::runtime_error("Error fork()");
			}
		}
		else if (!(isCGI(_target.substr(_target.find_last_of(".") + 1, _target.length()), server->cgi))
				&& isGlobalCGI(_target.substr(_target.find_last_of(".") + 1, _target.length()))
				&& _target[_target.length() - 1] != '/') {
					_response += request.getVersion() + ' ' + BAD_REQUEST + "\r\n";
					_response += "Content-Type: text/html\r\n\r\n";
					_response += (server->errors.find("400") != server->errors.end()) ? readFile(server->errors.at("400")) : "Not Found";
					_response += "\r\n";
					return;
				}

		std::cout << UCYN << "is CGI ? ---> " << isCGI(_target.substr(_target.find_last_of(".") + 1, _target.length()), server->cgi) << CRESET << std::endl;
		std::cout << "target: " << _target << std::endl;
		if (isDirectory(_target)) {

			if (request.getLocation()->directory_listing) {

				DIR *d;
				struct dirent *dir;
				d = opendir(_target.c_str());
				std::string uri = request.getUri();
				if (uri[uri.length() - 1] != '/') uri += '/';
				if (d) {
					_response += request.getVersion() + ' ' + OK + "\r\n";
					_response += "Content-Type: text/html\r\n\r\n";
					while ((dir = readdir(d)) != nullptr) {
						if (dir->d_name[0] == '.') {
							if (dir->d_name[1] == 0) continue;
							if (dir->d_name[1] == '.' && dir->d_name[2] == 0) {
								_response += "<a href=\"..\">Parent Directory</a><br/>\r\n";
								continue;
							}
						}
						_response += std::string("<a href=\"") + uri + dir->d_name + "\">" + dir->d_name + "</a><br/>\r\n";
					}
				}

			} else {
				_response += request.getVersion() + ' ' + FORBIDDEN + "\r\n";
				_response += "Content-Type: text/plain\r\n\r\n";
				_response += (server->errors.find("403") != server->errors.end()) ? readFile(server->errors.at("403")) : "Forbidden";
				_response += "\r\n";
			}

		} else if (isFile(_target)) {
			std::string filename = _target.substr(_target.find_last_of('/'));
			std::string extention = filename.substr(filename.find_last_of('.') + 1);

			std::cout << "method: " << request.getMethod() << std::endl;

			_response += request.getVersion() + ' ' + OK + "\r\n";
			_response += "Server: webserv\r\n";
			_response += "Connection: close\r\n";
			_response += "Content-Encoding: identity\r\n";
			_response += "Access-Control-Allow-Origin: *\r\n\r\n";

			try {
				_response += readFile(_target);
			} catch (std::exception const & e) {
				std::cerr << e.what() << std::endl;
				_response += e.what();
			}

			_response += "\r\n";

			// if (request.getMethod() == "POST") {

			// 	std::cout << "\e[1;35mfilname: " << filename << "\e[0m" << std::endl;
			// 	std::cout << "\e[1;36mextension " << extention << "\e[0m" << std::endl;

			// 	_response += "\r\n";

			// }

		} else {
			_response += request.getVersion() + ' ' + NOT_FOUND + "\r\n";
			_response += "Content-Type: text/html\r\n\r\n";
			_response += (server->errors.find("404") != server->errors.end()) ? readFile(server->errors.at("404")) : "Not Found";
			_response += "\r\n";
		}

	}

}

// void	Response::bodyParser(Request const & request, Server const * server) {



// }

void Response::write() {
	ssize_t bytes_sent = send(_fd, _response.c_str(), ((BUFFER_SIZE <= _response.length()) ? BUFFER_SIZE : (_response.length() % BUFFER_SIZE)), 0);
	if (bytes_sent == -1) throw std::runtime_error("send() failed");
	if (_response.empty()) _finished = true;
	_response.erase(0, bytes_sent);
	std::cout << "\e[1;34m" << bytes_sent << "\e[0m" << " bytes sent" << std::endl;
}

bool Response::isFinished() {
	return _finished;
}
