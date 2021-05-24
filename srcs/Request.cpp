#include "Request.hpp"
#include "Client.hpp"

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

const std::string&	Request::getTempBody(void) const
{
	return (this->temp_body);
}

Client*	Request::getClient(void)
{
	return (this->client);
}

///////////////////////////
/////////getter_end////////
///////////////////////////

void Request::setClient(Client *client)
{
	this->client = client;
}


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
	this->status = PARSING_HEADER;
	this->type = NOBODY;
}

bool	Request::tryMakeRequest(void)
{
	std::size_t	found = this->raw_request.find("\r\n\r\n");

	// std::cout << "first header\n" << raw_request << "hyeonski tkfkdgo\n" << std::endl;

	if (found != std::string::npos && this->status == PARSING_HEADER)
	{
		this->makeStartLine();
		this->makeRequestHeader();
		this->status = PARSING_BODY;
		int	res = bodyCheck();
		// exit(0);
		if (res == NOBODY)
		{
			// this->raw_request = this->temp_body;
			this->temp_body.clear();
			return (true);
		}
	}
	if (this->status == PARSING_BODY)
	{
		this->temp_body += raw_request;
		return (isComplete());
	}
	return (false);
}

///////////////// private func ///////////////////////

void	Request::makeStartLine(void)
{
	// std::cout <<"|" << this->raw_request  << "|" << std::endl;
	std::size_t	found = this->raw_request.find("\r\n");
	std::string start_line = this->raw_request.substr(0, found);

	this->parseMethod(start_line);
	this->parseUri(start_line);
	this->parseHttpVersion(start_line);

	if (this->raw_request.length() > (found + 2))
		this->raw_request = this->raw_request.substr(found + 2);
	else
		this->raw_request = "";
}

void	Request::parseMethod(std::string &start_line)
{
	this->method = start_line.substr(0, start_line.find(' '));
}

void	Request::parseUri(std::string &start_line)
{
	std::size_t start_pos = start_line.find(' ');

	this->uri = start_line.substr(start_pos + 1, start_line.rfind(' ') - start_pos - 1);
}

void	Request::parseHttpVersion(std::string &start_line)
{
	this->http_version = start_line.substr(start_line.rfind(' ') + 1);
}

void	Request::makeRequestHeader(void)
{
	this->raw_header = this->raw_request.substr(0, this->raw_request.find("\r\n\r\n"));
	
	std::vector<std::string> split_vec;

	ft_split(this->raw_header, "\r\n", split_vec);

	for (std::vector<std::string>::iterator i = split_vec.begin(); i != split_vec.end(); i++)
	{
		std::string temp = *i;
		std::size_t found = temp.find(":");
		std::string key = temp.substr(0, found);
		while (found + 1 < temp.length() && temp[found + 1] == 32) // space 건너뛰기
			found++;
		std::string value = "";
		if (temp.length() > (found + 1))
			value = temp.substr(found + 1);
		headers[key] = value;
	}

	// 맵 출력용
	std::cout << "size = " <<  headers.size() << std::endl;
	for (std::map<std::string, std::string>::iterator j = headers.begin(); j != headers.end(); j++)
		std::cout << "[" << j->first << "] value = [" << j->second << "]" << std::endl;

	this->raw_request = this->raw_request.substr(this->raw_request.find("\r\n\r\n") + 4);
}

bool	Request::bodyCheck(void)
{
	std::map<std::string, std::string>::iterator iter = this->headers.find("Transfer-Encoding");
	if (iter != this->headers.end() && iter->second == "chunked")
		return (this->type = CHUNKED);

	iter = this->headers.find("Content-Length");
	if (iter != this->headers.end() && iter->second != "")
		return (this->type = CONTENT_LENGTH);
	return (this->type);
}

bool	Request::isComplete(void)
{
	std::size_t len = ft_atoi(this->headers["Content-Length"]);

	if (this->type == CONTENT_LENGTH && this->temp_body.length() >= len)
	{
		this->raw_body += this->temp_body.substr(0, len);
		this->raw_request += this->temp_body.substr(len);
		temp_body.clear();
		this->status = PARSING_HEADER;
		return (true);
	}
	else if (this->type == CHUNKED)
	{
		std::size_t found = this->temp_body.find("\r\n");
		std::size_t	chunk_size;


		while  (found != std::string::npos)
		{
			chunk_size = ft_atoi_hex(this->temp_body.substr(0, found));
			if (chunk_size == 0)
			{
				if (temp_body.length() > 5) //0\r\n\r\n 말고 더 있으면
					this->raw_request += this->temp_body.substr(found + 4); // 마지막청크 다음 데이터 raw_request에 저장
				this->temp_body.clear();
				this->status = PARSING_HEADER;
				return (true);
			}
			//this->temp_body = this->temp_body.substr(found + 2);
			std::string cut = this->temp_body.substr(found + 2); //숫자 뒤 \r\n 다음부터 자름(데이터 보기 위해)
			if (cut.length() >= chunk_size + 2)
			{
				this->raw_body += cut.substr(0, chunk_size); // 데이터 청크사이즈만큼 붙여준다
				this->temp_body.clear();
				if (cut.length() > chunk_size + 2) // 뒤 청크 같이 들어온 경우 temp_body에 다음 애들 넣어주기
					this->temp_body = cut.substr(chunk_size + 2);
				raw_request.clear();
			}
			else 
				break ;
			found = this->temp_body.find("\r\n");
		}
	}
	return (false);
}
