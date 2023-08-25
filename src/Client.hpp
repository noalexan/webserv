#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <Config/Config.hpp>
# include <Request/Request.hpp>
# include <Response/Response.hpp>

struct Client {
	Server const * server;
	Request request;
	Response response;
};

#endif
