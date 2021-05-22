#include "../includes/Response.hpp"

Response::Response()
{
	this->status = DEFAULT_STATUS;
}

Response::~Response()
{

}

// Response::Response(const Response &src)
// {
// 	(void)src;
// }

// Response& Response::operator=(const Response &src)
// {
// 	(void)src;
// }

std::map<std::string, std::string>&	Response::getHeader(void) const
{
	return (this->header);
}

void		Response::tryMakeResponse(ResourceFD *resource_fd, int fd, Request& request)
{
	char	buf[BUFFER_SIZE];
	int		read_size;
	std::string	raw;
	
	if (resource_fd->getType() == CGI_RESOURCE_FDTYPE)
	{
		int status;
		if (waitpid(resource_fd->pid, &status, WNOHANG) == 0)
		{
			// CGI Process 안끝남
			return ;
		}
		while ((read_size = read(fd, buf, BUFFER_SIZE - 1)) != -1)
		{
			buf[read_size] = '\0';
		}
		if (read_size == -1)
			; // 500 Error
		this->makeCGIResponse(raw);
		raw.clear();
	}
	this->makeResponseHeader(request);

	
	// if ((read_size = read(fd, buf, BUFFER_SIZE - 1)) == -1)
	// 	; //internal server error
	// buf[read_size] = '\0';
}

void	Response::makeCGIResponse(std::string& raw)
{
	// status-line
	std::vector<std::string> status_line;
	std::size_t status_sep = raw.find("\r\n");
	ft_split(raw.substr(0, status_sep), " ", start_line);
	this->status = ft_atoi(start_line[1]);

	// Header
	std::vector<std::string> header_line;
	std::size_t header_sep = raw.find("\r\n\r\n");
	ft_split(raw.substr(status_sep + 2, header_sep - status_sep), " ", header_line);
	if (*(--header_line[1].end()) == ';')
		header_line[1].erase(--header_line[1].end());
	this->header.insert(std::pair<std::string, std::string>("Content-Type", header_line[1]));

	// Body

	this->body = raw.substr(header_sep + 2);
}

void		Response::makeResponseHeader(Request& request)
{
	// reqeust valid check -> Status Code

	if (status != 0 && status != 200)
		// 에러, 리다이렉션 체크
	// 헤더 기준
}