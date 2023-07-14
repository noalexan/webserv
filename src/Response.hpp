#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <iostream>
# include <unistd.h>
# include <sys/stat.h>
# include <sys/event.h>

# include "Request.hpp"

/* SECCESS */
# define OK						std::string("200 OK")
# define CREATED				std::string("201 CREATED")
# define ACCEPTED				std::string("202 ACCEPTED")
# define PARTIAL_INFORMATION	std::string("203 PARTIAL INFORMATION")
# define NO_RESPONSE			std::string("204 NO RESPONSE")
# define RESET_CONTENT			std::string("205 RESET CONTENT")
# define PARTIAL_CONTENT		std::string("206 PARTIAL CONTENT")

/* REDIRECT */
# define MOVED					std::string("301 MOVED")
# define FOUND					std::string("302 FOUND")
# define METHOD					std::string("303 METHOD")
# define NOT_MODIFIED			std::string("304 NOT MODIFIED")

/* CLIENT ERROR */
# define BAD_REQUEST			std::string("400 BAD REQUEST")
# define UNAUTHORIZED			std::string("401 UNAUTHORIZED")
# define PAYMENT_REQUIRED		std::string("402 PAYMENT_REQUIRED")
# define FORBIDDEN				std::string("403 FORBIDDEN")
# define NOT_FOUND				std::string("404 NOT FOUND")
# define METHOD_NOT_ALLOWED		std::string("405 METHOD NOT ALLOWED")

/* SERVER ERROR */
# define INTERNAL_ERROR			std::string("500 INTERNAL ERROR")
# define NOT_IMPLEMENTED		std::string("501 NOT IMPLEMENTED")
# define BAD_GATEWAY			std::string("502 BAD GATEWAY")
# define SERVICE_UNAVAILABLE	std::string("503 SERVICE UNAVAILABLE")
# define GATEWAY_TIMEOUT		std::string("504 GATEWAY TIMEOUT")

class Response {

	private:

		std::string const	_version;
		std::string	const	_method;
		std::string	const	_filePath;

		std::string			_finalResponse;

	public:

		Response( Request const & request, std::string const & filePath );

		bool pathExists( std::string const & path ) const;
		bool isDirectory( std::string const & path ) const;

		std::string const & getResponse( void ) const { return ( _finalResponse ); }

};

#endif
