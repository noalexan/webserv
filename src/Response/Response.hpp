#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <string>
# include <Request/Request.hpp>

/* SUCCESS */
# define OK						"200 OK"
# define CREATED				"201 CREATED"
# define ACCEPTED				"202 ACCEPTED"
# define PARTIAL_INFORMATION	"203 PARTIAL INFORMATION"
# define NO_RESPONSE			"204 NO RESPONSE"
# define RESET_CONTENT			"205 RESET CONTENT"
# define PARTIAL_CONTENT		"206 PARTIAL CONTENT"

/* REDIRECT */
# define MOVED					"301 MOVED"
# define FOUND					"302 FOUND"
# define METHOD					"303 METHOD"
# define NOT_MODIFIED			"304 NOT MODIFIED"

/* CLIENT ERROR */
# define BAD_REQUEST			"400 BAD REQUEST"
# define UNAUTHORIZED			"401 UNAUTHORIZED"
# define PAYMENT_REQUIRED		"402 PAYMENT_REQUIRED"
# define FORBIDDEN				"403 FORBIDDEN"
# define NOT_FOUND				"404 NOT FOUND"
# define METHOD_NOT_ALLOWED		"405 METHOD NOT ALLOWED"
# define PAYLOAD_TOO_LARGE		"413 PAYLOAD TOO LARGE"

/* SERVER ERROR */
# define INTERNAL_ERROR			"500 INTERNAL ERROR"
# define NOT_IMPLEMENTED		"501 NOT IMPLEMENTED"
# define BAD_GATEWAY			"502 BAD GATEWAY"
# define SERVICE_UNAVAILABLE	"503 SERVICE UNAVAILABLE"
# define GATEWAY_TIMEOUT		"504 GATEWAY TIMEOUT"

class Response {

	protected:
		int			_fd;
		std::string	_response;
		bool		_finished;

	public:
		Response();

		void setFd(int fd);
		void handle(Request const &);
		void write();
		bool isFinished();

};

#endif
