#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <iostream>
# include <map>
# include <vector>
# include "Manager.hpp"
# include "Type.hpp"
# include "Request.hpp"

# define DEFAULT_STATUS 0 // 0으로 하면 되려나?

class Response
{
	private	:
		std::string start_line;
		std::map<std::string, std::string> header;
		std::string body;

		int status;
	public	:
		Response();
		//Response(const Response &src);
		//Response& operator=(const Response &src);
		virtual	~Response();

		std::map<std::string, std::string>&	getHeader(void);

		void	tryMakeResponse(ResourceFD *resource_fd, int fd, Request& request);
		void	makeCGIResponse(std::string& raw);
		void	makeResponseHeader(Request &request);
};

#endif