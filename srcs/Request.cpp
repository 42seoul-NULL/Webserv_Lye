#include "../includes/Request.hpp"

Request::Request(void)
{
	this->raw_request.clear();
	initRequest();
}

Request::Request(const Request& src) : raw_request(src.raw_request), method(src.method), uri(src.uri), http_version(src.http_version), raw_header(src.raw_header), raw_body(src.raw_body), temp_body(src.temp_body), status(src.status), type(src.type)
{
	this->headers.insert(src.headers.begin(), src.headers.end());
}

Request&	Request::operator=(const Request& src)
{
	this->raw_request.clear();
	initRequest();

	this->raw_request = src.raw_request;
	this->method = src.method;
	this->uri = src.uri;
	this->http_version = src.http_version;
	this->headers.insert(src.headers.begin(), src.headers.end());

	this->raw_header = src.raw_header;
	this->raw_body = src.raw_body;
	this->temp_body = src.temp_body;
	this->status = 0;
	this->type = 0;

	return (*this);
}

///////////////////////////
/////////getter////////////
///////////////////////////


std::string&	Request::getRawRequest(void)
{
	return (this->raw_request);
}

const std::string&	Request::getMethod(void) const
{
	return (this->method);
}

const std::string&	Request::getUri(void) const
{
	return (this->uri);
}

const std::string&	Request::getHttpVersion(void) const
{
	return (this->http_version);
}

std::map<std::string, std::string>&	Request::getHeaders(void) const
{
	return (const_cast<std::map<std::string, std::string>& > (this->headers));
}

const	std::string&	Request::getRawHeader(void) const
{
	return (this->raw_header);
}

const	std::string&	Request::getRawBody(void) const
{
	return (this->raw_body);
}

///////////////////////////
/////////getter_end////////
///////////////////////////


/////////////////////////// public func /////////////////////

void	Request::initRequest(void)
{
	this->method.clear();
	this->uri.clear();
	this->http_version.clear();
	this->headers.clear();
	this->raw_header.clear();
	this->raw_body.clear();
	this->temp_body.clear();
	status = 0;
	type = 0;
}

bool	Request::tryMakeRequest(void)
{
	std::size_t	found = this->raw_request.find("\r\n\r\n");
	int	res = 1;

	std::cout << "first header\n" << raw_request << "hyeonski tkfkdgo\n" << std::endl;
	if (found != std::string::npos && this->status == 0)
	{
		this->makeStartLine();
		this->makeRequestHeader();
		this->status = 1;
		res = bodyCheck();
		exit(0);
		if (res == 0)
		{
			// this->raw_request = this->temp_body;
			this->temp_body.clear();
			return (true);
		}
	}
	if (this->status == 1)
	{
		this->temp_body += raw_request;
		return (isComplete());
	}
	return (false);
}

///////////////// private func ///////////////////////

void	Request::makeStartLine(void)
{
	std::cout <<"|" << this->raw_request  << "|" << std::endl;
	this->parseMethod();
	this->parseUri();
	this->parseHttpVersion();
	std::size_t n = this->raw_request.find("\r\n");
	if (this->raw_request.length() >= (n + 2))
		this->raw_request = this->raw_request.substr(n + 2);
	else
		this->raw_request = "";
}

void	Request::parseMethod(void)
{
	std::size_t	found = this->raw_request.find("\r\n");
	std::string start_line = this->raw_request.substr(0, found);
	this->method = start_line.substr(0, start_line.find(' '));
}

void	Request::parseUri(void)
{
	std::size_t	found = this->raw_request.find("\r\n");
	std::string start_line = this->raw_request.substr(0, found);
	std::size_t start_pos = start_line.find(' ');

	this->uri = start_line.substr(start_pos + 1, start_line.find_last_of(' ') - start_pos - 1);
}

void	Request::parseHttpVersion(void)
{
	std::size_t	found = this->raw_request.find("\r\n");
	std::string start_line = this->raw_request.substr(0, found);

	this->http_version = start_line.substr(start_line.find_last_of(' ') + 1);
}

void	Request::makeRequestHeader(void)
{
	size_t rn = this->raw_request.find("\r\n");
	this->raw_header = this->raw_request.substr(0, this->raw_request.find("\r\n\r\n"));
	
	std::vector<std::string> split_vec;

	std::cout << "------------------------\n";
	std::cout << raw_header << "\n--------------------------------------" << std::endl;
	ft_split(this->raw_header, "\r\n", split_vec);
	std::cout << "vec size = " << split_vec.size() << std::endl;
	std::vector<std::string>::iterator i;
	for (i = split_vec.begin(); i != split_vec.end(); i++)
	{
		std::string temp = *i;
		std::size_t found = temp.find(":");
		std::string key = temp.substr(0, found);
		while (temp[found + 1] == 32)
			found++;
		std::string value = "";
		if (temp.length() != (found + 1))
			value = temp.substr(found + 1);
		headers[key] = value;
	}

	// 맵 출력용
	std::cout << "size = " <<  headers.size() << std::endl;
	for (std::map<std::string, std::string>::iterator j = headers.begin(); j != headers.end(); j++)
		std::cout << "[" << j->first << "] value = [" << j->second << "]" << std::endl;

	this->raw_request = this->raw_request.substr(this->raw_request.find("\r\n\r\n") + 4);
	this->raw_request.clear();
}

bool	Request::bodyCheck(void)
{
	if (this->headers["Transfer-Encoding"] == "chunked")
		this->type = CHUNKED;
	else if (this->headers["Content-Length"] != "")
		this->type = CONTENT_LENGTH;
	return (this->type);
}

bool	Request::isComplete(void)
{
	std::size_t len = ft_atoi(this->headers["Content-Length"]);

	if (this->type == 1 && this->temp_body.length() >= len)
	{
		this->raw_body += this->temp_body.substr(0, len);
		this->raw_request += this->temp_body.substr(len); // 다음 리퀘스트가 한 번에 붙어서 오면 어떻게 처리해야하는가?
		temp_body.clear();
		return (true);
	}
	else if (this->type == 2)
	{
		std::size_t found = this->temp_body.find("\r\n");
		std::size_t	chunk_size;

		while  (found != std::string::npos)
		{
			chunk_size = ft_atoi_hex(this->temp_body.substr(0, found));
			if (chunk_size == 0)
			{
				this->raw_request += this->temp_body.substr(found + 2);
				this->temp_body.clear();
				return (true);
			}
			//this->temp_body = this->temp_body.substr(found + 2);
			std::string cut = this->temp_body.substr(found + 2);
			if (cut.length() >= chunk_size)
			{
				found = cut.find("\r\n");
				this->raw_body += cut.substr(0, found);
				this->temp_body.clear();
				if (cut.length() >= found + 2)
					this->temp_body = cut.substr(found + 2);
			}
			else 
				break ;
			found = this->temp_body.find("\r\n");
		}
	}
	return (false);
}
