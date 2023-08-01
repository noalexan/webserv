#include <Response/Response.hpp>
#include <deque>
#include <sys/stat.h>

bool	isDirectory( std::string const & path ) {
	struct stat path_stat;
	stat(path.c_str(), &path_stat);
	return ( S_ISDIR(path_stat.st_mode) );
}

bool	isFile( std::string const & path ) {
	struct stat path_stat;
	stat(path.c_str(), &path_stat);
	return ( S_ISREG(path_stat.st_mode) );
}

Response::Response( Request const & request, int const & clientfd, Config const & config ) : _clientfd( clientfd ) {

	time_t		tmm = time(0);
	tm			*ltm = localtime(&tmm);
	tm			*gmt = gmtime(&tmm);
	std::string	date = ctime(&tmm);
	std::ostringstream	oss;
	oss << std::put_time(gmt, "%H:%M:%S");

	_target = request.getTarget();

	if (isDirectory(_target) || _target[_target.length() - 1] == '/') {
		if (_target[_target.length() - 1] != '/') _target += '/';
		
		for (std::deque<std::string>::const_iterator it = request.getLocation()->indexes.begin(); it != request.getLocation()->indexes.end(); it++) {
			if (isFile(_target + *it)) {
				_target += *it;
				break;
			}
		}

	}

	if (isDirectory(_target)) {

		if (request.getLocation()->directoryListing) {
			*this << request.getVersion() + ' ' + OK + "\r\n";
			*this << "Content-Type: text/plain\r\n\r\n";
			*this << "Directory Listing\r\n";
		} else {
			*this << request.getVersion() + ' ' + FORBIDDEN + "\r\n";
			*this << "Content-Type: text/plain\r\n\r\n";
			*this << "Forbidden\r\n";
		}

	} else if (isFile(_target)) {

		std::string extention = _target.substr(_target.find_last_of('.'), _target.length());

		if (extention == ".php") {

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
				dup2(pipefd[1], STDOUT_FILENO);

				std::map<std::string, std::string> env = config.getEnvironment();

				env["HTTP_HOST"] = request.getHeaders().find("Host")->second;
				env["SCRIPT_FILENAME"] = _target;

				char **environment = new char*[env.size() + 1];
				int i = 0;
				for (std::map<std::string, std::string>::iterator it = env.begin(); it != env.end(); it++) {
					environment[i] = new char[it->first.size() + it->second.size() + 2];
					strcpy(environment[i], (it->first + "=" + it->second).c_str());
					i++;
				}
				environment[env.size()] = nullptr;

				execve("/usr/local/bin/php-cgi", (char *[]) {(char *) "/usr/local/bin/php-cgi", (char *) _target.c_str(), nullptr}, environment);
				exit(EXIT_FAILURE);
			} else {
				close(pipefd[1]);
				char buffer[4096];
				int bytes_read;

				std::string _;
				std::map<std::string, std::string> headers;

				std::cout << "reading php response..." << std::endl;
				while ((bytes_read = read(pipefd[0], buffer, 4095)) > 0) {
					buffer[bytes_read] = '\0';
					_ += buffer;
				}

				std::cout << "parsing php response..." << std::endl;
				std::istringstream iss(_);
				std::string line;

				while (std::getline(iss, line)) {
					if (line == "") {
						break;
					}

					if (line.find(": ") != std::string::npos) {
						headers[line.substr(0, line.find(": "))] = line.substr(line.find(": ") + 2, line.length() - 2);
						std::cout << "header: " << line.substr(0, line.find(": ")) << ": " << line.substr(line.find(": ") + 2, line.length() - 2) << std::endl;
					}
				}

				std::cout << "sending php response..." << std::endl;
				*this << request.getVersion() + ' ' + ((headers.find("Status") != headers.end()) ? headers.find("Status")->second : OK) + "\r\n";
				*this << "Server: webserv\r\n";

				*this << "Parameters:";
				for (std::map<std::string, std::string>::const_iterator it = request.getParams().begin(); it != request.getParams().end(); it++)
					*this << " " + it->first + "=" + it->second + ";";
				*this << "\r\n";

				for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++) {
					if (it->first != "Status") {
						*this << it->first + ": " + it->second + "\n";
					}
				}

				*this << "\r\n";
				*this << _.substr(_.find("\r\n\r\n") + 4, _.length());

				std::cout << "php response sent" << std::endl;

			}

		} else {

			_headers["Server"] = "webserv";

			std::map<std::string, std::string> const & contentTypes = config.getContentTypes();
			if (contentTypes.find(extention) != contentTypes.end())
				_headers["Content-Type"] = contentTypes.at(extention);

			_headers["Date"]	= date.substr(0, date.find_first_of(' ')) + ", " // * Day Week
								+ std::to_string(ltm->tm_mday) + ' ' // * Day
								+ date.substr(date.find_first_of(' ') + 1, date.find_first_of(' ')) + ' ' // * Month
								+ std::to_string(1900 + ltm->tm_year) + ' ' // * Year
								+ oss.str() // * GMT hour
								+ "GMT";

			*this << "Parameters:";
			for (std::map<std::string, std::string>::const_iterator it = request.getParams().begin(); it != request.getParams().end(); it++)
				*this << " " + it->first + "=" + it->second + ";";
			*this << "\r\n";

			_headers["Conection"] = "close";
			_headers["Content-Encoding"] = "identity";
			_headers["Access-Control-Allow-Origin"] = '*';

			std::cout << "target: " << _target.substr( _target.find_last_of('/') + 1, _target.length() ) << std::endl;

			*this << request.getVersion() + ' ' + OK + "\r\n";
			for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); it++)
				*this << it->first + ": " + it->second + "\r\n";
			*this << "\r\n";

			*this << readFile(_target);
			*this << "\r\n";

		}

	} else {
		*this << request.getVersion() + ' ' + NOT_FOUND + "\r\n";
		*this << "Content-Type: text/plain\r\n\r\n";
		*this << "Not Found : " + _target + "\r\n";
	}
}

void	Response::operator<<( std::string const & o ) { write(_clientfd, o.c_str(), o.length()); }

std::string	Response::readFile( std::string path ) const {
	std::ifstream file(path);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return ( content );
}
