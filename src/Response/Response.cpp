#include <Response/Response.hpp>
#include <deque>
#include <sys/stat.h>

static std::string readFile(std::string path) {
	std::ifstream file(path);
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

Response::Response( Request const & request, int const & clientfd ): _clientfd(clientfd) {

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

	std::cout << "target: " << _target << std::endl;

	if (isDirectory(_target)) {

		if (request.getLocation()->directoryListing) {
			*this << request.getVersion() + ' ' + OK + "\r\n";
			*this << "Content-Type: text/html\r\n\r\n";

			*this << "Directory Listing\r\n";
			*this << "<a href=\"https://google.com/\" target=\"_blank\">google</a>\r\n";

		} else {
			*this << request.getVersion() + ' ' + FORBIDDEN + "\r\n";
			*this << "Content-Type: text/plain\r\n\r\n";
			*this << "Forbidden\r\n";
		}

	} else {

		*this << request.getVersion() + ' ' + OK + "\r\n\r\n";
		*this << readFile(_target);

	}

}

void	Response::operator<<( std::string const & o ) { write(_clientfd, o.c_str(), o.length()); }
