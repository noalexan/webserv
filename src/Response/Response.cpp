#include <ExitCode.hpp>
#include <Response/Response.hpp>
#include <deque>
#include <sys/stat.h>

#define BUFFER_SIZE 512

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
			*this << request.getVersion() + ' ' + "403 FORBIDDEN\r\n";
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

			pid_t pid = fork();

			if (pid == -1) {
				throw std::runtime_error("fork() failed");
			}

			if (pid == 0) {

				dup2(pipefd[1], STDOUT_FILENO);
				close(pipefd[0]);
				close(pipefd[1]);

				std::map<std::string, std::string> const & params = request.getParams();

				char * argv[3 + params.size()];

				argv[0] = (char *) "/usr/local/bin/php-cgi";
				argv[1] = (char *) _target.c_str();

				int i = 2;

				for (std::map<std::string, std::string>::const_iterator it = params.begin(); it != params.end(); it++) {
					std::string param = it->first + "=" + it->second;
					argv[i] = (char *) param.c_str();
					i++;
				}

				argv[i] = nullptr;

				std::string http_host = "HTTP_HOST=192.168.1.69";
				std::string script_filename = "SCRIPT_FILENAME=" + _target;

				char * env[3 + params.size()];

				env[0] = (char *) http_host.c_str();
				env[1] = (char *) script_filename.c_str();

				i = 2;

				for (std::map<std::string, std::string>::const_iterator it = params.begin(); it != params.end(); it++) {
					std::string param = it->first + "=" + it->second;
					env[i] = (char *) param.c_str();
					i++;
				}

				env[i] = nullptr;

				execve(argv[0], argv, env);

				exit(CGI_FAILURE);

			} else {

				close(pipefd[1]);

				// WNOHANG  ->   Ne pas bloquer si aucun fils ne s’est terminé.

				int status;
				waitpid(pid, &status, 0 /* WNOHANG ? */);

				if (status != 0) {
					throw std::runtime_error("Error: CGI failed");
				}

				char buffer[BUFFER_SIZE + 1];
				ssize_t bytes_read;

				std::string _;

				do {
					
					bytes_read = read(pipefd[0], buffer, BUFFER_SIZE);

					if (bytes_read == -1) {
						throw std::runtime_error("Error: read() failed");
					}

					buffer[bytes_read] = '\0';
					_.append(buffer);

				} while (bytes_read == BUFFER_SIZE);

				if (_.find("\r\n\r\n") != std::string::npos) {

					std::map<std::string, std::string> headers;
					std::string body = _.substr(_.find("\r\n\r\n") + 4, _.length());
					_ = _.substr(0, _.find("\r\n\r\n"));

					std::istringstream iss(_);
					std::string line;

					while (std::getline(iss, line)) {
						std::string key = line.substr(0, line.find_first_of(':'));
						std::string value = line.substr(line.find_first_of(':') + 2, line.length());
						headers[key] = value;
					}

					if (headers.find("Status") != headers.end()) {
						*this << request.getVersion() + ' ' + headers["Status"] + "\r\n";
					} else {
						*this << request.getVersion() + ' ' + OK + "\r\n";
					}

					for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++) {
						if (it->first != "Status") {
							*this << it->first + ": " + it->second + "\n";
						}
					}

					*this << "\r\n";
					*this << body;

				} else {
					*this << request.getVersion() + ' ' + OK + "\r\n\r\n";
					*this << _;
				}

			}

		} else {

			*this << request.getVersion() + " " + OK + "\r\n";

			_headers["Server"] = "webserv";

			std::map<std::string, std::string> const & contentTypes = config.getContentTypes();
			if (contentTypes.find(extention) != contentTypes.end())
				_headers["Content-Type"] = contentTypes.at(extention);

			_headers["Date"]	= date.substr(0, date.find_first_of(' ')) + ", " // * Day Week
								+ std::to_string(ltm->tm_mday) + ' ' // * Day
								+ date.substr(date.find_first_of(' ') + 1, date.find_first_of(' ')) + ' ' // * Month
								+ std::to_string(1900 + ltm->tm_year) + ' ' // * Year
								+ oss.str() + ' ' // * GMT hour
								+ "GMT";

			_headers["Conection"] = "close";
			_headers["Content-Encoding"] = "identity";
			_headers["Access-Control-Allow-Origin"] = '*';

			*this << request.getVersion() + ' ' + OK + "\r\n";

			for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); it++) {
				*this << it->first + ": " + it->second + "\r\n";
			}
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

void	Response::operator<<( std::string const & o ) { write(_clientfd, o.c_str(), o.length()); std::cout << o << std::flush; }

std::string	Response::readFile( std::string path ) const {
	std::ifstream file(path);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return ( content );
}
