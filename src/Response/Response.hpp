#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include <string>
#include <Request/Request.hpp>

/* SUCCESS */
#define OK                  "200 OK"
#define CREATED             "201 CREATED"
#define ACCEPTED            "202 ACCEPTED"
#define PARTIAL_INFORMATION "203 PARTIAL INFORMATION"
#define NO_CONTENT          "204 NO CONTENT"
#define RESET_CONTENT       "205 RESET CONTENT"
#define PARTIAL_CONTENT     "206 PARTIAL CONTENT"

/* REDIRECT */
#define MOVED               "301 MOVED"
#define FOUND               "302 FOUND"
#define METHOD              "303 METHOD"
#define NOT_MODIFIED        "304 NOT MODIFIED"

/* CLIENT ERROR */
#define BAD_REQUEST         "400 BAD REQUEST"
#define UNAUTHORIZED        "401 UNAUTHORIZED"
#define PAYMENT_REQUIRED    "402 PAYMENT_REQUIRED"
#define FORBIDDEN           "403 FORBIDDEN"
#define NOT_FOUND           "404 NOT FOUND"
#define METHOD_NOT_ALLOWED  "405 METHOD NOT ALLOWED"
#define REQUEST_TIMEOUT     "408 REQUEST TIMEOUT"
#define PAYLOAD_TOO_LARGE   "413 PAYLOAD TOO LARGE"

/* SERVER ERROR */
#define INTERNAL_ERROR      "500 INTERNAL ERROR"
#define NOT_IMPLEMENTED     "501 NOT IMPLEMENTED"
#define BAD_GATEWAY         "502 BAD GATEWAY"
#define SERVICE_UNAVAILABLE "503 SERVICE UNAVAILABLE"
#define GATEWAY_TIMEOUT     "504 GATEWAY TIMEOUT"

class Response {

	protected:
		std::string _response;
		int         _fd;
		bool        _finished;
		bool        _body;

	public:
		Response();

		void setFd(int const & fd);
		void handle(Request const &, Server const * server, Config const & config, bool const & timeout);
		void write();

		bool const & isFinished() const;

		void responseMaker(Server const * const server, std::string const & statusCode, std::string const & statusHeader);
		void responseMaker(Server const * const server, std::string const & statusCode, std::string const & statusHeader, std::string const & redirection);
		void CGIMaker(Server const * server, std::string const & extension, std::string const & target, std::string const & statusHeader);

};

#endif
