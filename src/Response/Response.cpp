#include <Response/Response.hpp>

Response::Response( Request const & request, std::string const & filePath ) : _version( request.version() + ' ' ), _filePath( filePath ) {

	// for ( std::map<std::string, std::string>::const_iterator it = request.headers().begin(); it != request.headers().end(); it++ )
	// 	std::cout << "{" << it->first << ", " << it->second << "}" << std::endl;
	
	std::cout << "method: " << request.method() << std::endl;
	std::cout << "uri: " << request.uri() << std::endl;
	std::cout << "version: " << request.version() << std::endl;

	std::cout << "\r\e[K\e[1;36mSending response...\e[0m" << std::flush;

	if ( request.method() != "GET" ) {
		_finalResponse = _version + METHOD_NOT_ALLOWED + "\r\n\r\n405 Method Not Allowed\r\n";
	}

	else if (!pathExists(filePath)) {
		std::cout << "\r\e[K\e[1;35m" << _filePath << " doesn't exists\e[0m" << std::endl;
		_finalResponse = _version + NOT_FOUND + "\r\n\r\n404 Not Found\r\n";
	}

	if (isDirectory(filePath)) {
		std::cout << "\r\e[K\e[1;35m" << filePath << " is a Directory\e[0m" << std::endl;
		_finalResponse = _version + FORBIDDEN + "\r\n\r\n403 Forbidden\r\n";
	}

}

bool Response::pathExists( std::string const & path ) const {
	return ( access(path.c_str(), R_OK) != -1 );
}

bool	Response::isDirectory( std::string const & path ) const {
	struct stat st;
	if (stat(path.c_str(), &st) == 0) {
		return ( S_ISDIR(st.st_mode) );
	}
	return ( false );
}
