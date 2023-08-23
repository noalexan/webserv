#include <Response/Response.hpp>
#include <ExitCode.hpp>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#define BUFFER_SIZE 256

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

Response::Response(Request const & request, int const & clientfd, Server const & server): _clientfd(clientfd) {

	// *this << request.getVersion() + ' ' + OK + "\r\n";
	// *this << "Content-Type: text/plain\r\n\r\n";
	// *this << "Hello, World!\r\n";
	// return;

	// time_t		tmm = time(0);
	// tm			*ltm = localtime(&tmm);
	// tm			*gmt = gmtime(&tmm);
	// std::string	date = ctime(&tmm);
	// std::ostringstream	oss;
	// oss << std::put_time(gmt, "%H:%M:%S");

	std::string	line;
	_target = request.getTarget();

	if (request.getMethod() == "POST") {

		for (std::map<std::string, std::string>::const_iterator it = server.uploads.begin(); it != server.uploads.end(); it++) {
			std::cout << it->first << ", " << it->second << std::endl;

			std::string const & body = request.getBody();

			std::cout << body << std::endl;

			if (request.getHeaders().find("Content-Type") != request.getHeaders().end()) {
				std::string content_type = request.getHeaders().at("Content-Type");
				std::string boundary = "--" + content_type.substr(content_type.find("boundary=") + 9);
				boundary.pop_back();
				std::string content = body.substr(body.find(boundary) + boundary.length() + 1, body.find(boundary + "--") - boundary.length() - 2);
				std::string file_headers = content.substr(0, content.find("\r\n\r\n"));
				std::string file_content = content.substr(content.find("\r\n\r\n") + 4);
				file_content.pop_back();
				std::cout << "\e[31;1m" << file_headers << "\e[0m" << std::endl;
				std::cout << "\e[32;1m" << file_content << "\e[0m" << std::endl;
				std::string filename = _target + '/' + file_headers.substr(file_headers.find("filename=\"") + 10);
				filename = filename.substr(0, filename.find_first_of("\"\r\n"));
				std::cout << filename << std::endl;
				std::ofstream file(filename);
				std::string extention = filename.substr(filename.find_last_of('.') + 1);
				file << file_content;
				*this << request.getVersion() + ' ' + OK + "\r\n\r\n";
				*this << content;
				return;
			}
		}

	}

	if (isDirectory(_target) || _target[_target.length() - 1] == '/') {
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
			if (uri[uri.length() - 1] != '/') uri += '/';
			if (d) {
				*this << request.getVersion() + ' ' + OK + "\r\n";
				*this << "Content-Type: text/html\r\n\r\n";
				while ((dir = readdir(d)) != nullptr) {
					if (dir->d_name[0] == '.') {
						if (dir->d_name[1] == 0) continue;
						if (dir->d_name[1] == '.' && dir->d_name[2] == 0) {
							*this << "<a href=\"..\">Parent Directory</a><br/>\r\n";
							continue;
						}
					}
					*this << std::string("<a href=\"") + uri + dir->d_name + "\">" + dir->d_name + "</a><br/>\r\n";
				}
			}

		} else {
			*this << request.getVersion() + ' ' + FORBIDDEN + "\r\n";
			*this << "Content-Type: text/plain\r\n\r\n";
			*this << "Forbidden\r\n";
		}

	} else if (isFile(_target) && access(_target.c_str(), R_OK) + 1) {

		std::string filename = _target.substr(_target.find_last_of('/'));
		std::string extention = filename.substr(filename.find_last_of('.') + 1);

		for (std::map<std::string, std::string>::const_iterator it = server.cgi.begin(); it != server.cgi.end(); it++) {
			std::cout << extention << " == " << it->first << std::endl;
			if (extention == it->first) {
				std::cout << it->second << std::endl;

				int pipefd[2];

				if (pipe(pipefd) == -1) {
					throw std::runtime_error("pipe() failed");
				}

				int pid = fork();

				if (pid == -1) {
					throw std::runtime_error("fork() failed");
				}

				if (pid == 0) {
					close(pipefd[0]);
					dup2(pipefd[1], 1);
					execve(it->second.c_str(), (char *[]) {(char *) it->second.c_str(), (char *) _target.c_str()}, nullptr);
					exit(1);
				}

				close(pipefd[1]);

				std::string _;
				char buffer[BUFFER_SIZE + 1];
				ssize_t bytes_read;

				do {

					bytes_read = read(pipefd[0], buffer, BUFFER_SIZE);

					if (bytes_read == -1) {
						continue;
						throw std::runtime_error("read() failed");
					}

					buffer[BUFFER_SIZE] = 0;
					_.append(buffer);

				} while (bytes_read == BUFFER_SIZE);

				std::cout << "\e[33;1m" << _ << "\e[0m" << std::endl;

				*this << request.getVersion() + ' ' + OK;
				*this << "\r\n\r\n";
				*this << _;
				*this << "\r\n";

				close(pipefd[0]);

				return;
			}
		}

		*this << request.getVersion() + ' ' + OK + "\r\n";
		*this << "Server: webserv\r\n";

		// *this << "Date: "	+ date.substr(0, date.find_first_of(' ')) + ", " // * Day Week
		// 					+ std::to_string(ltm->tm_mday) + ' ' // * Day
		// 					+ date.substr(date.find_first_of(' ') + 1, date.find_first_of(' ')) + ' ' // * Month
		// 					+ std::to_string(1900 + ltm->tm_year) + ' ' // * Year
		// 					+ oss.str() + ' ' // * GMT hour
		// 					+ "GMT\r\n";

		*this << "Connection: close\r\n";
		*this << "Content-Encoding: identity\r\n";
		*this << "Access-Control-Allow-Origin: *\r\n\r\n";

		try {
			*this << readFile(_target);
		} catch (std::exception const & e) {
			std::cerr << e.what() << std::endl;
			*this << e.what();
		}

		*this << "\r\n";

	} else {
		*this << request.getVersion() + ' ' + NOT_FOUND + "\r\n";
		*this << "Content-Type: text/plain\r\n\r\n";
		*this << "Not Found\r\n";
	}

}

void	Response::operator<<( std::string const & o ) { send(_clientfd, o.c_str(), o.length(), 0); }
