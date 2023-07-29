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

Response::Response( Request const & request, int const & clientfd ) : _clientfd( clientfd ) {

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
		} else {
			*this << request.getVersion() + ' ' + FORBIDDEN + "\r\n";
		}
	} else if (isFile(_target)) {
		*this << request.getVersion() + ' ' + OK + "\r\n";
	} else {
		*this << request.getVersion() + ' ' + NOT_FOUND + "\r\n";
	}

	_headers["Server"] = "webserv";

	*this << request.getVersion() + ' ' + OK + "\r\n";
	for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); it++)
		*this << it->first + ": " + it->second + "\r\n";
	*this << "\r\n";

	*this << readIndexFile(_target);
	*this << "\r\n";

}

void	Response::operator<<( std::string const & o ) { write(_clientfd, o.c_str(), o.length()); }

std::string	Response::readIndexFile( std::string path ) const {
	std::ifstream file(path);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return ( content );
}
