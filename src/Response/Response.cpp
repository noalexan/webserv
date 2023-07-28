#include <Response/Response.hpp>

Response::Response( Request const & request, std::string const & filePath ) : _version( request.getVersion() + ' ' ), _filePath( "index.html" ) {

	// for ( std::map<std::string, std::string>::const_iterator it = request.headers().begin(); it != request.headers().end(); it++ )
	// 	std::cout << "{" << it->first << ", " << it->second << "}" << std::endl;
	
	std::cout << "method: " << request.getMethod() << std::endl;
	std::cout << "uri: " << request.getUri() << std::endl;
	std::cout << "version: " << request.getVersion() << std::endl;

	std::cout << "\r\e[K\e[1;36mSending response...\e[0m" << std::endl;

	if ( request.getMethod() != "GET" ) {
		_finalResponse = _version + METHOD_NOT_ALLOWED + "\r\n\r\n405 Method Not Allowed\r\n";
	}

	if (!pathExists(filePath)) {
		std::cout << "\r\e[K\e[1;35m" << _filePath << " doesn't exists\e[0m" << std::endl;
		_finalResponse = _version + NOT_FOUND + "\r\n\r\n404 Not Found\r\n";
		return ;
	}

	// if (isDirectory(filePath)) {
	// 	std::cout << "\r\e[K\e[1;35m" << filePath << " is a Directory\e[0m" << std::endl;
	// 	_finalResponse = _version + FORBIDDEN + "\r\n\r\n403 Forbidden\r\n";
	// }

	_finalResponse = _version + OK + "\r\n\r\n" + readIndexFile( filePath ) + "\r\n\r\n";

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

std::string	Response::readIndexFile( std::string path )
{
	std::ifstream file(path);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	std::cout << "content : " << content << std::endl;
	return content;
}
