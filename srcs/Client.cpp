#include "../includes/Client.hpp"

Client::Client()
{
	this->server_socket_fd = -1;
	this->socket_fd = -1;
	this->status = REQUEST_RECEIVING;
}

Client::Client(int server_socket_fd, int socket_fd) : server_socket_fd(server_socket_fd), socket_fd(socket_fd)
{

}

Client::~Client()
{
	
}

// Client::Client(const Client &src)
// {
// 	status = src.status;
// 	server_socket_fd = src.server_socket_fd;
// 	socket_fd = src.socket_fd;
// 	remain_body = src.remain_body;
// 	last_request_ms = src.last_request_ms;
// 	request = src.request;
// 	response = src.response;
// }

// Client& Client::operator=(const Client &src)
// {
// 	status = src.status;
// 	server_socket_fd = src.server_socket_fd;
// 	socket_fd = src.socket_fd;
// 	remain_body = src.remain_body;
// 	last_request_ms = src.last_request_ms;
// 	request = src.request;
// 	response = src.response;
// }

void	Client::setLastRequestMs(unsigned long last_request_ms)
{
	this->last_request_ms = last_request_ms;
}

void	Client::setStatus(t_status status)
{
	this->status = status;
	return ;
}

void	Client::setServerSocketFd(int server_socket_fd)
{
	this->server_socket_fd = server_socket_fd;
	return ;
}

void	Client::setSocketFd(int socket_fd)
{
	this->socket_fd = socket_fd;
	return ;
}

void	Client::setRemainBody(long long remain_body)
{
	this->remain_body = remain_body;
	return ;
}

void	Client::setServer(Server &server)
{
	this->server = &server;
}

t_status	Client::getStatus()
{
	return (this->status);
}

int			Client::getServerSocketFd()
{
	return (this->server_socket_fd);
}

Request		&Client::getRequest()
{
	return (this->request);
}

Response	&Client::getResponse()
{
	return (this->response);
}

int		Client::getSocketFd()
{
	return (this->socket_fd);
}

long long		Client::getRemainBody()
{
	return (this->remain_body);
}

unsigned long	Client::getLastRequestMs()
{
	return (this->last_request_ms);
}

Server		*Client::getServer()
{
	return (this->server);
}

void Client::readRequest(void)
{
	char buf[BUFFER_SIZE + 1];

	int readed;
	readed = read(this->socket_fd, buf, BUFFER_SIZE);
	buf[readed] = 0;

	this->request.getRawRequest() += buf;

	if (this->request.tryMakeRequest() == true)
		this->status = REQUEST_COMPLETE;
}
