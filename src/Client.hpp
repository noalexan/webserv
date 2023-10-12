#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <Config/Config.hpp>
#include <Request/Request.hpp>
#include <Response/Response.hpp>
#include <ctime>

struct Client {
	Server const * server;
	Request request;
	Response response;
	time_t timestamp;
};

#endif
