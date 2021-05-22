#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <iostream>
# include <vector>
# include <map>
# include "../libft_cpp/libft.hpp"
# include "Client.hpp"

// # define NEEDBODY -1	// 이건 사용하는 건지 안하는 건지 몰랑몰랑 아 몰라드 아몰라몰라 아몰레드
# define CHUNKED 2
# define CONTENT_LENGTH 1
# define NOBODY 0

# define PARSING_HEADER 0
# define PARSING_BODY 1



class Request
{
	private:
		std::string	raw_request;

		std::string	method;
		std::string	uri;
		std::string	http_version;
		std::map<std::string, std::string> headers;

		std::string raw_header;
		std::string	raw_body;

		std::string temp_body;
		int	status;
		int	type;
		Client	*client;

	public:
		Request(void);
		Request(const Request& src);
		virtual ~Request(void){};
		Request& operator=(const Request& src);

		std::string&	getRawRequest(void);
		const std::string&	getMethod(void) const;
		const std::string&	getUri(void) const;
		const std::string&	getHttpVersion(void) const;
		std::map<std::string, std::string>&	getHeaders(void) const;
		const	std::string&	getRawHeader(void) const;
		const std::string&	getRawBody(void) const;
		const std::string& getTempBody(void) const;
		Client*	getClient(void) const;

	public:
		void	initRequest(void);
		bool	tryMakeRequest(void);

	private:
		void	makeStartLine(void);
		void	makeRequestHeader(void);

		void	parseMethod(std::string &start_line);
		void	parseUri(std::string &start_line);
		void	parseHttpVersion(std::string &start_line);

		bool	bodyCheck(void);
		bool	isComplete(void);		
};

#endif
