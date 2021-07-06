#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <iostream>
# include <map>
# include <vector>

# define DEFAULT_STATUS 0

class Request;
class Location;
class ResourceFD;
class Client;

class Response
{
	private:
		std::string start_line;
		std::multimap<std::string, std::string> headers;
		std::string body;
		int status;
		std::string raw_response;
		Client *client;
		bool	seek_flag;
		std::string cgi_raw;
		unsigned long long file_size;
		size_t res_idx;
		bool is_writing;

	public:
		Response();
		virtual	~Response();

		std::multimap<std::string, std::string>&	getHeaders(void);
		std::string &getRawResponse(void);
		Client *getClient();
		std::string &getBody(void);
		size_t getResIdx(void);
		bool getWriting(void);

		void setClient(Client *client);
		void setResIdx(size_t res_idx);
		void setWriting(bool is_writing);
		void	tryMakeResponse(ResourceFD *resource_fd, int fd, Request& request);
		void	makePutResponse(Request &request);
		void	makeDeleteResponse(Request &request);
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
		void	makeAutoIndexResponse(std::string &path, const std::string &uri);
		
		void	makeRawResponse(void);
		void	initResponse(void);
		void	generateErrorPage(int status);
		void	generateSessionCookie(void);

};

#endif