#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <string>
# include <map>

class Request {

	protected:
		bool			_finished;
		std::string		_request;
		int				_fd;

		std::string		_method;
		std::string		_uri;
		std::string		_version;
		std::map<std::string, std::string>	_headers;

	public:
		Request();

		void setFd( int fd );
		void read();
		bool isFinished();
		void parse();

		void print();

};

#endif
