#include "Response.hpp"
#include "../libft_cpp/libft.hpp"
#include "Request.hpp"
#include "Type.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "Manager.hpp"
#include "Location.hpp"
#include <list>
#include <dirent.h>

Response::Response() : seek_flag(false)
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

std::string	&Response::getBody(void)
{
	return (this->body);
}

void		Response::setClient(Client *client)
{
	this->client = client;
}

void		Response::tryMakeResponse(ResourceFD *resource_fd, int fd, Request& request)
{
	char	buf[BUFFER_SIZE];
	int		read_size;
	std::string	cgi_raw;
	
	if (resource_fd->getType() == CGI_RESOURCE_FDTYPE)
	{
		int status;
		if (waitpid(resource_fd->pid, &status, WNOHANG) == 0) // CGI Process 안끝남
			return ;
		if (seek_flag == false)
		{
			lseek(fd, 0, SEEK_SET);
			this->seek_flag = true;
		}
		read_size = read(fd, buf, BUFFER_SIZE - 1);
		if (read_size > 0) // stat file size check 해서 작성하자....
		{
			buf[read_size] = '\0';
			cgi_raw += std::string(buf);
		}
		delete resource_fd;
		Manager::getInstance()->getFDTable()[fd] = NULL;
		Manager::getInstance()->getFDTable().erase(fd);
		FT_FD_CLR(fd, &(Manager::getInstance()->getReads()));
		FT_FD_CLR(fd, &(Manager::getInstance()->getErrors()));
		close(fd);
		if (read_size == -1)
		{
			this->makeErrorResponse(500, NULL); // 500 Error
			return ;
		}
		this->applyCGIResponse(cgi_raw); // status 저장, content_type 저장, body 저장
		this->makeCGIResponseHeader(request);
		this->makeStartLine();
		this->makeRawResponse();
		request.getClient()->setStatus(RESPONSE_COMPLETE);
	}
	else
	{
		this->body.clear();
		while ((read_size = read(fd, buf, BUFFER_SIZE - 1)) > 0)
		{
			buf[read_size] = '\0';
			this->body += std::string(buf);
		}
		delete resource_fd;
		Manager::getInstance()->getFDTable()[fd] = NULL;
		Manager::getInstance()->getFDTable().erase(fd);
		FT_FD_CLR(fd, &(Manager::getInstance()->getReads()));
		FT_FD_CLR(fd, &(Manager::getInstance()->getErrors()));
		close(fd);
		if (read_size == -1)
		{
			this->makeErrorResponse(500, NULL); // 500 Error
			return ;
		}
		this->status = 200;
		this->makeResponseHeader(request);
		this->makeStartLine();
		this->makeRawResponse();
		request.getClient()->setStatus(RESPONSE_COMPLETE);
	}
}

void	Response::applyCGIResponse(std::string& cgi_raw)
{
	// status-line
	std::vector<std::string> status_line;
	std::size_t status_sep = cgi_raw.find("\r\n");
	ft_split(cgi_raw.substr(0, status_sep), " ", status_line);
	for (std::vector<std::string>::const_iterator iter = status_line.begin(); iter != status_line.end(); iter++)
	{
		std::cout << *iter << std::endl;
	}
	this->status = ft_atoi(status_line[1]);

	// Header
	std::vector<std::string> header_line;
	std::size_t header_sep = cgi_raw.find("\r\n\r\n");
	ft_split(cgi_raw.substr(status_sep + 2, header_sep - status_sep), " ", header_line);
	if (*(--header_line[1].end()) == ';')
		header_line[1].erase(--header_line[1].end());
	this->headers.insert(std::pair<std::string, std::string>("Content-Type", header_line[1]));

	// Body
	this->body = cgi_raw.substr(header_sep + 2);
}



void		Response::makeResponseHeader(Request& request)
{
	this->generateDate();
	this->generateLastModified(request);
	this->generateContentLanguage();
	this->generateContentLocation(request);
	this->generateContentType(request);
	this->generateServer();
	this->generateContentLength();

}

void		Response::makeCGIResponseHeader(Request& request)
{
	this->generateDate();
	this->generateContentLanguage();
	this->generateContentLocation(request);
	this->generateContentType(request);
	this->generateServer();
	this->generateContentLength();
}

void	Response::generateAllow(Request& request)
{
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

void	Response::generateContentLength(void)
{
	this->headers.insert(std::pair<std::string, std::string>("Content-Length", ft_itoa(this->body.length())));
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
	this->seek_flag = false;
}

void	Response::makeErrorResponse(int status, Location *location)
{
	this->status = status;
	this->headers.insert(std::pair<std::string, std::string>("Content-Type", "text/html"));
	this->generateDate();
	this->generateServer();
	if (location == NULL || location->getErrorPages().count(status) == 0)
	{
		this->generateErrorPage(status);
		this->headers.insert(std::pair<std::string, std::string>("Content-Length", ft_itoa(this->body.length())));
		if (status == 401)
			this->generateWWWAuthenticate();
		this->makeStartLine();
		this->makeRawResponse();
		this->client->setStatus(RESPONSE_COMPLETE);
		return ;
	}
	else
	{
		int fd = open(location->getErrorPages()[status].c_str(), O_RDONLY);
		if (fd == -1)
		{
			makeErrorResponse(500, NULL);
			return ;
		}
		ResourceFD *error_resource = new ResourceFD(ERROR_RESOURCE_FDTYPE, this->client);
		Manager::getInstance()->getFDTable().insert(std::pair<int, FDType*>(fd, error_resource));
		FT_FD_SET(fd, &(Manager::getInstance()->getReads()));
		FT_FD_SET(fd, &(Manager::getInstance()->getErrors()));
		if (Manager::getInstance()->getWebserver().getFDMax() < fd)
		{
			Manager::getInstance()->getWebserver().setFDMax(fd);
		}
	}
}

void	Response::makeAutoIndexResponse(std::string &path)
{
	DIR		*dir_ptr;
	struct dirent	*file;

	if((dir_ptr = opendir(path.c_str())) == NULL)
	{
		this->makeErrorResponse(500, NULL);
		return ;
	}

	this->body += "<html>\r\n";
	this->body += "<head>\r\n";
	this->body += "<title>Index of " + path + "</title>\r\n";
	this->body += "</head>\r\n";
	this->body += "<body bgcolor=\"white\">\r\n";
	this->body += "<h1>Index of " + path + "</h1>\r\n";
	this->body += "<hr>\r\n";
	this->body += "<pre>\r\n";

	while((file = readdir(dir_ptr)) != NULL)
	{
		struct stat	sb;
		struct tm*	timeinfo;
		char buffer[4096];
		std::string name = std::string(file->d_name);
		if (file->d_type == 4)
			name += '/';
		this->body += "<a href=\"" + name + "\">" + name + "</a>\r\n";

		if (stat(path.c_str(), &sb) == -1)
		{
			this->start_line.clear();
			this->body.clear();
			this->makeErrorResponse(500, NULL);
			return ;
		}
		timeinfo = localtime(&sb.st_mtime);
		strftime(buffer, 4096, "%d-%b-%Y %H:%M", timeinfo);
		this->body += "\"                                        " + std::string(buffer) + "                   ";
		if (S_ISDIR(sb.st_mode))
			this->body += "-\"\r\n";
		else
		{
			this->body.erase(this->body.length() - ft_itoa(sb.st_size).length() + 1);
			this->body += ft_itoa(sb.st_size) + "\"\r\n";
		}
	}

	this->body += "</pre>\r\n";
	this->body += "<hr>\r\n";
	this->body += "</body>\r\n";
	this->body += "</html>";
	
	this->status = 200;
	this->headers.insert(std::pair<std::string, std::string>("Content-Type", "text/html"));
	this->generateDate();
	this->generateServer();
	this->generateContentLength();
	
	this->makeStartLine();
	this->makeRawResponse();
}

void		Response::generateErrorPage(int status)
{
	this->status = status;

	this->body.clear();
	this->body += "<html>\r\n";
	this->body += "<head>\r\n";
	this->body += "<title>" + ft_itoa(this->status) + " " + Manager::getInstance()->getStatusCode().find(ft_itoa(this->status))->second + "</title>\r\n";
	this->body += "</head>\r\n";
	this->body += "<body bgcolor=\"white\">\r\n";
	this->body += "<center>\r\n";
	this->body += "<h1>" + ft_itoa(this->status) + " " + Manager::getInstance()->getStatusCode().find(ft_itoa(this->status))->second + "</h1>\r\n";
	this->body += "</center>\r\n";
	this->body += "<hr>\r\n";
	this->body += "<center>HyeonSkkiDashi/1.0</center>\r\n";
	this->body += "</body>\r\n";
	this->body += "</html>";
}