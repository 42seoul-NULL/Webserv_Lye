#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <iostream>
# include <map>
# include <vector>

# define DEFAULT_STATUS 0 // 0으로 하면 되려나?

class Request;
class Location;
class ResourceFD;
class Client;

class Response
{
	private:
		std::string start_line;
		std::map<std::string, std::string> headers;
		std::string body;
		int status;
		std::string raw;
		Client *client;
		bool	seek_flag;

	public:
		Response();
		//Response(const Response &src);
		//Response& operator=(const Response &src);
		virtual	~Response();

		std::map<std::string, std::string>&	getHeaders(void);
		std::string &getRawResponse(void);
		Client *getClient();
		std::string &getBody(void);

		void setClient(Client *client);

		void	tryMakeResponse(ResourceFD *resource_fd, int fd, Request& request);
		void	applyCGIResponse(std::string& raw);
		void	makeResponseHeader(Request &request);
		void	makeCGIResponseHeader(Request& request);
		void	generateAllow(Request& request);
		void	generateDate(void);
		void	generateLastModified(Request& request);
		void	generateContentLanguage(void);
		void	generateContentLocation(Request &request);
		void	generateContentLength(void);
		void	generateContentType(Request &request);
		void	generateLocation(Location &loc);
		void	generateRetryAfter(void);
		void	generateServer(void);
		void	generateWWWAuthenticate();

		void	makeRedirectResponse(Location &location);
		void	makeStartLine();
		void	makeErrorResponse(int status, Location *location);
		void	makeAutoIndexResponse(std::string &path);
		// void	makeCGIStartLine();
		
		void	makeRawResponse(void);
		void	initResponse(void);
		void	generateErrorPage(int status);

};

#endif