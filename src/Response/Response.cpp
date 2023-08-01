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

Response::Response( Request const & request, int const & clientfd, std::map<std::string, std::string> const & contentTypes ) : _clientfd( clientfd ) {

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
		} else {
			*this << request.getVersion() + ' ' + FORBIDDEN + "\r\n";
		}
	} else if (isFile(_target)) {
		*this << request.getVersion() + ' ' + OK + "\r\n";
	} else {
		*this << request.getVersion() + ' ' + NOT_FOUND + "\r\n";
	}

	std::string extention = _target.substr(_target.find_last_of('.'), _target.length());

	// ? ** RESPONSE INFO **

	_headers["Server"] = "webserv";

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

	// ? *******************

	std::cout << "target: " << _target.substr( _target.find_last_of('/') + 1, _target.length() ) << std::endl;

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
