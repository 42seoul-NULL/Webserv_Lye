#include "Response.hpp"
#include "utils.hpp"
#include "Request.hpp"
#include "Type.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "Manager.hpp"
#include "Location.hpp"
#include <list>
#include <dirent.h>

Response::Response()
{
	this->status = DEFAULT_STATUS;
	this->client = NULL;
	this->res_idx = 0;
}

Response::~Response()
{

}

std::multimap<std::string, std::string>&	Response::getHeaders(void)
{
	return (this->headers);
}

std::string &Response::getRawResponse(void)
{
	return (this->raw_response);
}

std::string	&Response::getBody(void)
{
	return (this->body);
}

size_t Response::getResIdx(void)
{
	return (this->res_idx);
}

void		Response::setClient(Client *client)
{
	this->client = client;
}

void Response::setResIdx(size_t res_idx)
{
	this->res_idx = res_idx;
}

void		Response::tryMakeResponse(ResourceFD *resource_fd, int fd, Request& request, long to_read)
{
	char	buf[BUFFER_SIZE];
	int		read_size;
	
	if (resource_fd->getType() == CGI_RESOURCE_FDTYPE)
	{
		read_size = read(fd, buf, BUFFER_SIZE - 1);
		if (read_size == -1)
		{
			std::cerr << "temporary resource read error!" << std::endl;
			return ;
		}
		buf[read_size] = '\0';
		this->cgi_raw += buf;
		if (read_size != 0)
			return ;
		clrFDonTable(fd, FD_RDONLY);
		this->applyCGIResponse(this->cgi_raw); // status, content_type, body
		this->makeCGIResponseHeader(request);
		this->makeStartLine();
		this->makeRawResponse();
		request.getClient()->setStatus(RESPONSE_COMPLETE);
	}
	else
	{
		read_size = read(fd, buf, BUFFER_SIZE - 1);
		if (read_size == -1)
		{
			clrFDonTable(fd, FD_RDONLY);
			this->makeErrorResponse(500, NULL); // 500 Error
			return ;
		}
		buf[read_size] = '\0';
		this->body += std::string(buf);
		if (to_read - read_size > 0)
			return ;

		clrFDonTable(fd, FD_RDONLY);
		this->status = 200;
		this->makeResponseHeader(request);
		this->makeStartLine();
		if (this->client->getRequest().getMethod() == "HEAD")
			this->body.clear();
		this->makeRawResponse();
		request.getClient()->setStatus(RESPONSE_COMPLETE);
	}
}

void		Response::makePutResponse(Request &request)
{
	(void)request;
	this->status = 201;
	this->generateDate();
	this->generateServer();
	this->generateContentLength();
	this->makeStartLine();
	this->makeRawResponse();
	this->client->setStatus(RESPONSE_COMPLETE);
}

void		Response::makeDeleteResponse(Request &request)
{
	this->status = 200;
	this->generateDate();
	this->generateServer();
	this->generateContentLocation(request);
	this->generateContentType(request);
	this->generateContentLength();
	this->makeStartLine();
	this->makeRawResponse();
	this->client->setStatus(RESPONSE_COMPLETE);
}

void	Response::applyCGIResponse(std::string& cgi_raw)
{
	// status-line
	std::vector<std::string> status_line;
	std::size_t status_sep = cgi_raw.find("\r\n");
	ft_split(cgi_raw.substr(0, status_sep), " ", status_line);
	this->status = atoi(status_line[1].c_str());

	// Header
	std::vector<std::string> header_line;
	std::size_t header_sep = cgi_raw.find("\r\n\r\n");
	ft_split(cgi_raw.substr(status_sep + 2, header_sep - status_sep - 2), " ", header_line);
	if (*(--header_line[1].end()) == ';')
		header_line[1].erase(--header_line[1].end());
	this->headers.insert(std::pair<std::string, std::string>("Content-Type", header_line[1]));

	// Body
	if (cgi_raw.length() > header_sep + 4)
		this->body = cgi_raw.substr(header_sep + 4);
	else
		this->body = "";
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
	#ifdef BONUS
		this->generateSessionCookie();
	#endif

}

void		Response::makeCGIResponseHeader(Request& request)
{
	this->generateDate();
	this->generateContentLanguage();
	this->generateContentLocation(request);
	this->generateServer();
	this->generateContentLength();

	#ifdef BONUS
		this->generateSessionCookie();
	#endif
}

void	Response::generateAllow(Request& request)
{
	std::string allow;
	Location &test = MANAGER->getWebserver().getPerfectLocation(*request.getClient()->getServer(), request.getUri());
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
	strftime(buffer, 4096, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
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
	size_t pos = ext.rfind('.');
	if (pos == std::string::npos)
		this->headers.insert(std::pair<std::string, std::string>("Content-Type", "application/octet-stream"));
	else
	{
		ext = ext.substr(pos);
		if (MANAGER->getMimeType().count(ext) == 0)
			this->headers.insert(std::pair<std::string, std::string>("Content-Type", "application/octet-stream"));		
		else			
		{
			std::string type = MANAGER->getMimeType()[ext];
			this->headers.insert(std::pair<std::string, std::string>("Content-Type", type));
		}
	}
}

void	Response::generateLocation(Location &location)
{
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
	this->status = location.getRedirectReturn();
	this->generateDate();
	this->generateLocation(location);
	this->generateRetryAfter();
	this->makeStartLine();

	this->makeRawResponse();
	this->client->setStatus(RESPONSE_COMPLETE);
}

void	Response::makeStartLine()
{
	std::map<std::string, std::string>::const_iterator iter = MANAGER->getStatusCode().find(ft_itoa(this->status));
	if (iter == MANAGER->getStatusCode().end())
		return ;
	this->start_line += "HTTP/1.1 ";
	this->start_line += ft_itoa(this->status);
	this->start_line += " ";
	this->start_line += iter->second;
}

void	Response::makeRawResponse(void)
{
	this->raw_response += this->start_line;
	this->raw_response += "\r\n";

	for (std::map<std::string, std::string>::const_iterator iter = this->headers.begin(); iter != this->headers.end(); iter++)
	{
		this->raw_response += iter->first;
		this->raw_response += ": ";
		this->raw_response += iter->second;
		this->raw_response += "\r\n";
	}
	this->raw_response += "\r\n";

	this->raw_response += this->body;
}

void	Response::initResponse(void)
{
	this->start_line.clear();
	this->headers.clear();
	this->body.clear();
	this->raw_response.clear();
	this->status = DEFAULT_STATUS;
	this->cgi_raw.clear();
	this->res_idx = 0;
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
		if (this->client->getRequest().getMethod() == "HEAD")
			this->body.clear();
		if (status == 401)
			this->generateWWWAuthenticate();

		#ifdef BONUS
			this->generateSessionCookie();
		#endif

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
		setFDonTable(fd, FD_RDONLY, error_resource);
	}
}

void	Response::makeAutoIndexResponse(std::string &path, const std::string &uri)
{
	DIR *dir_ptr;
	struct dirent *file;

	if((dir_ptr = opendir(path.c_str())) == NULL)
	{
		this->makeErrorResponse(500, NULL);
		return ;
	}

	this->body += "<html>\r\n";
	this->body += "<head>\r\n";
	this->body += "<title>Index of " + uri + "</title>\r\n";
	this->body += "</head>\r\n";
	this->body += "<body bgcolor=\"white\">\r\n";
	this->body += "<h1>Index of " + uri + "</h1>\r\n";
	this->body += "<hr>\r\n";
	this->body += "<pre>\r\n";

	while ((file = readdir(dir_ptr)) != NULL)
	{
		struct stat	sb;
		struct tm*	timeinfo;
		char buffer[4096];
		std::string name = std::string(file->d_name);
		if (file->d_type == DT_DIR)
			name += '/';
		this->body += "<a href=\"" + name + "\">" + name + "</a>\r\n";

		if (stat((path + name).c_str(), &sb) == -1)
		{
			this->start_line.clear();
			this->body.clear();
			this->makeErrorResponse(500, NULL);
			return ;
		}
		timeinfo = localtime(&sb.st_mtime);
		strftime(buffer, 4096, "%d-%b-%Y %H:%M", timeinfo);
		this->body += "                                        " + std::string(buffer) + "                   ";
		if (S_ISDIR(sb.st_mode))
			this->body += "-\r\n";
		else
		{
			this->body.erase(this->body.length() - ft_itoa(sb.st_size).length() + 1);
			this->body += ft_itoa(sb.st_size) + "\r\n";
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
	#ifdef BONUS
		this->generateSessionCookie();
	#endif
	
	this->makeStartLine();
	this->makeRawResponse();
	this->client->setStatus(RESPONSE_COMPLETE);
}

void	Response::makeLogResponse(void)
{
	std::list<std::string> &logs = this->client->getServer()->getSessionLogs()[this->client->getSessionId()];

	this->body += "<html>\r\n";
	this->body += "<head>\r\n";
	this->body += std::string("<title>") + "HyeonSkkiDashi/1.0 Log" + "</title>\r\n";
	this->body += "</head>\r\n";
	this->body += "<body bgcolor=\"white\">\r\n";
	this->body += std::string("<h1>") + "Requested URL" + "</h1>\r\n";
	this->body += "<hr>\r\n";
	for (std::list<std::string>::iterator it = logs.begin(); it != logs.end(); it++)
	{
		this->body += *it + std::string("<br>");
		this->body += "\r\n";
	}
	this->body += "<hr>\r\n";
	this->body += "<center>HyeonSkkiDashi/1.0</center>\r\n";
	this->body += "</body>\r\n";
	this->body += "</html>";

	this->status = 200;
	this->generateDate();
	this->generateContentLength();
	this->generateServer();
	this->headers.insert(std::pair<std::string, std::string>("Content-Type", "text/html"));
	
	this->makeStartLine();
	this->makeRawResponse();
	this->client->setStatus(RESPONSE_COMPLETE);
}

void		Response::generateErrorPage(int status)
{
	this->status = status;

	this->body.clear();
	this->body += "<html>\r\n";
	this->body += "<head>\r\n";
	this->body += "<title>" + ft_itoa(this->status) + " " + MANAGER->getStatusCode().find(ft_itoa(this->status))->second + "</title>\r\n";
	this->body += "</head>\r\n";
	this->body += "<body bgcolor=\"white\">\r\n";
	this->body += "<center>\r\n";
	this->body += "<h1>" + ft_itoa(this->status) + " " + MANAGER->getStatusCode().find(ft_itoa(this->status))->second + "</h1>\r\n";
	this->body += "</center>\r\n";
	this->body += "<hr>\r\n";
	this->body += "<center>HyeonSkkiDashi/1.0</center>\r\n";
	this->body += "</body>\r\n";
	this->body += "</html>";
}

void	Response::generateSessionCookie(void)
{
	if (this->client->getSessionFlag() == true)
	{
		time_t t;
		char buffer[4096];
		struct tm* timeinfo;

		t = (time(NULL) + 24 * 60 * 60);
		timeinfo = localtime(&t);
		strftime(buffer, 4096, "%A, %d-%b-%Y %H:%M:%S GMT", timeinfo);

		std::string cookie;
		cookie += ("webserv_session_id=" + ft_itoa(this->client->getSessionId()));
		cookie += "; ";
		cookie += "path=/; ";
		cookie += ("expires=" + std::string(buffer));
		cookie += "; ";
		
		this->headers.insert(std::pair<std::string, std::string>("Set-Cookie", cookie));
		this->client->setSessionFlag(false);
	}
}
