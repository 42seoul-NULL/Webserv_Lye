#include "Response.hpp"
#include "../libft_cpp/libft.hpp"
#include "Request.hpp"
#include "Type.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "Manager.hpp"
#include "Location.hpp"
#include <list>

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

std::map<std::string, std::string>&	Response::getHeaders(void)
{
	return (this->headers);
}

std::string &Response::getRawResponse(void)
{
	return (this->raw);
}

void		Response::tryMakeResponse(ResourceFD *resource_fd, int fd, Request& request)
{
	char	buf[BUFFER_SIZE];
	int		read_size;
	std::string	raw;
	
	if (resource_fd->getType() == CGI_RESOURCE_FDTYPE)
	{
		this->body.clear();
		int status;
		if (waitpid(resource_fd->pid, &status, WNOHANG) == 0) // CGI Process 안끝남
			return ;
		while ((read_size = read(fd, buf, BUFFER_SIZE - 1)) != -1)
		{
			buf[read_size] = '\0';
			body += std::string(buf);
		}
		delete resource_fd;
		Manager::getInstance()->getFDTable()[fd] = NULL;
		Manager::getInstance()->getFDTable().erase(fd);
		FT_FD_CLR(fd, &(Manager::getInstance()->getReads()));
		FT_FD_CLR(fd, &(Manager::getInstance()->getErrors()));
		close(fd);
		if (read_size == -1)
		{
			this->makeErrorResponse(500); // 500 Error
			return ;
		}
			
		this->applyCGIResponse(raw);
		raw.clear();
	}
	else
	{
		this->body.clear();
		while ((read_size = read(fd, buf, BUFFER_SIZE - 1)) != -1)
		{
			buf[read_size] = '\0';
			body += std::string(buf);
		}
		delete resource_fd;
		Manager::getInstance()->getFDTable()[fd] = NULL;
		Manager::getInstance()->getFDTable().erase(fd);
		FT_FD_CLR(fd, &(Manager::getInstance()->getReads()));
		FT_FD_CLR(fd, &(Manager::getInstance()->getErrors()));
		close(fd);
		if (read_size == -1)
		{
			this->makeErrorResponse(500); // 500 Error
			return ;
		}

	}

	this->makeResponseHeader(request);
	this->makeStartLine();

	// if ((read_size = read(fd, buf, BUFFER_SIZE - 1)) == -1)
	// 	; //internal server error
	// buf[read_size] = '\0';
}

void	Response::applyCGIResponse(std::string& raw)
{
	// status-line
	std::vector<std::string> status_line;
	std::size_t status_sep = raw.find("\r\n");
	ft_split(raw.substr(0, status_sep), " ", status_line);
	this->status = ft_atoi(status_line[1]);

	// Header
	std::vector<std::string> header_line;
	std::size_t header_sep = raw.find("\r\n\r\n");
	ft_split(raw.substr(status_sep + 2, header_sep - status_sep), " ", header_line);
	if (*(--header_line[1].end()) == ';')
		header_line[1].erase(--header_line[1].end());
	this->headers.insert(std::pair<std::string, std::string>("Content-Type", header_line[1]));

	// Body

	this->body = raw.substr(header_sep + 2);
	this->makeRawResponse();
}

void		Response::makeResponseHeader(Request& request)
{
	// reqeust valid check -> Status Code

	if (status != 0 && status != 200)
		; // 에러, 리다이렉션 체크
	// 헤더 기준
}

void	Response::generateAllow(Request& request)
{
	
	// std::map<std::string, Location>::iterator location_iter = server.getLocations().find(request.getUri());
	//std::map<std::string, Location>::iterator location_iter 
	// if (location_iter != server.getLocations().end())
	// {
	// 	std::list<std::string>::iterator iter = location_iter->second.getAllowMethods().begin();
	// 	for (; iter != location_iter->second.getAllowMethods().end(); iter++)
	// 	{
	// 		// Request에서 URI 처리 후 맞는 Location_Iter Key 찾은 후 작성
	// 		this->allow += *iter;
	// 		if (iter != --location_iter->second.getAllowMethods().end())
	// 		{
	// 			this->allow += ", ";
	// 		}
	// 	}
	// }
	// else
	// {
	// 	Manager::getInstance()->getWebserver().getPerfectLocation(*request.getClient()->getServer(), request.getUri());
	// 	std::cout << "Not found Server" << std::endl;
	// }

	std::string allow;
	Location &test = Manager::getInstance()->getWebserver().getPerfectLocation(*request.getClient()->getServer(), request.getUri());
	for (std::list<std::string>::const_iterator iter = test.getAllowMethods().begin(); iter != test.getAllowMethods().end(); iter++)
	{
		allow += *iter;
		if (iter != --test.getAllowMethods().end())
			allow += ", ";
	}
	this->headers.insert(std::pair<std::string, std::string>("Allow", "allow"));
}

void	Response::generateDate(void)
{
	// Date 함수 살펴 본 후 작성하자.
	// 메시지가 만들어진 날짜, 객체 생성 시의 시간? 보내기 전 Raw화 하기전의 시간?
	time_t t;
	char buffer[4096];
	struct tm* timeinfo;

	t = time(NULL);
	timeinfo = localtime(&t);
	strftime(buffer, 4096, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
	this->headers.insert(std::pair<std::string, std::string>("Date", std::string(buffer)));
}

void	Response::generateLastModified(Request& request)
{
	// 파일의 최종 수정 시간.
	// stat 함수 체크하자.
	// URI 에 대한 파싱 진행 완료 후 stat 함수를 통해 정보를 읽은 후 수정 날짜를 규격에 맞춰서 작성하면 될듯
	struct stat	sb;
	struct tm*	timeinfo;
	char buffer[4096];
	(void)request;

	if (stat(request.getPath().c_str(), &sb) == -1)
	{
		std::cerr << "err" << std::endl;
		return ;
	}
	timeinfo = localtime(&sb.st_mtime);
	strftime(buffer, 4096, "%a, %d %b %Y %H:%M:%S GMT", timeinfo); // 연도 잘 안나옴
	this->headers.insert(std::pair<std::string, std::string>("Last-Modified", std::string(buffer)));
}

void	Response::generateContentLanguage(void)
{
	this->headers.insert(std::pair<std::string, std::string>("Content-Language", "ko-kr"));
}

void	Response::generateContentLocation(Request &request)
{
	this->headers.insert(std::pair<std::string, std::string>("Content-Location", request.getPath()));
}

void	Response::generateContentType(Request &request)
{
	std::string ext = request.getPath();
	size_t pos = ext.find('.');
	if (pos == std::string::npos) // 확장자가 없다.
		this->headers.insert(std::pair<std::string, std::string>("Content-Type", "application/octet-stream"));
	else
	{
		ext = ext.substr(pos);
		if (Manager::getInstance()->getMimeType().count(ext) == 0)
			this->headers.insert(std::pair<std::string, std::string>("Content-Type", "application/octet-stream"));		
		else			
		{
			std::string type = Manager::getInstance()->getMimeType()[ext];
			this->headers.insert(std::pair<std::string, std::string>("Content-Type", type));		
		}
	}
}

void	Response::generateLocation(Location &location)
{
	// 300번대 체크 ㄱㄱ
	// 201 
	this->status = location.getRedirectReturn();
	this->headers.insert(std::pair<std::string, std::string>("Location", location.getRedirectAddr()));
}

void	Response::generateRetryAfter(void)
{
	this->headers.insert(std::pair<std::string, std::string>("Retry-After", "100"));
}

void	Response::generateServer(void)
{
	this->headers.insert(std::pair<std::string, std::string>("Server", "HyeonSkkiDashi/1.0"));
}

void	Response::generateWWWAuthenticate()
{
	this->headers.insert(std::pair<std::string, std::string>("WWW-Authenticate", "Basic realm=\"Give me ID:PASS encoded base64\""));
}

void	Response::makeRedirectResponse(Location &location)
{
	this->generateDate();
	this->generateLocation(location);
	this->generateRetryAfter();
	this->makeStartLine();

	this->makeRawResponse();
}

void	Response::makeStartLine()
{

	std::map<std::string, std::string>::const_iterator iter = Manager::getInstance()->getStatusCode().find(ft_itoa(this->status));
	if (iter == Manager::getInstance()->getStatusCode().end())
		;
	this->start_line += "HTTP/1.1 ";
	this->start_line += ft_itoa(this->status);
	this->start_line += " ";
	this->start_line += iter->second;
	// this->start_line += "\r\n";
	// HTTP/1.1 200 OK
}

void	Response::makeRawResponse(void)
{
	this->raw += this->start_line;
	this->raw += "\r\n";

	for (std::map<std::string, std::string>::const_iterator iter = this->headers.begin(); iter != this->headers.end(); iter++)
	{
		this->raw += iter->first;
		this->raw += ": ";
		this->raw += iter->second;
		this->raw += "\r\n";
	}
	this->raw += "\r\n";

	this->raw += this->body;
}

void	Response::initResponse(void)
{
	this->start_line.clear();
	this->body.clear();
	this->raw.clear();
	this->headers.clear();
	this->status = DEFAULT_STATUS;
}
