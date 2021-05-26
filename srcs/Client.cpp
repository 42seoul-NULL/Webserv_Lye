#include "../includes/Client.hpp"

Client::Client()
{
	this->server_socket_fd = -1;
	this->socket_fd = -1;
	this->status = REQUEST_RECEIVING;
	this->request.setClient(this);
	this->response.setClient(this);
	this->last_request_ms = 0;
	this->server = NULL;
}

Client::Client(int server_socket_fd, int socket_fd) : server_socket_fd(server_socket_fd), socket_fd(socket_fd)
{
	this->status = REQUEST_RECEIVING;
	this->request.setClient(this);
	this->response.setClient(this);
	this->last_request_ms = 0;
	this->server = NULL;
}

Client::~Client()
{
	
}

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

unsigned long	Client::getLastRequestMs()
{
	return (this->last_request_ms);
}

Server		*Client::getServer()
{
	return (this->server);
}

int Client::readRequest(void)
{
	this->setLastRequestMs(ft_get_time());
	char buf[BUFFER_SIZE];

	int readed;
	readed = read(this->socket_fd, buf, BUFFER_SIZE - 1);
	if (readed <= 0)
		return (DISCONNECT_CLIENT);
	buf[readed] = 0;

	this->request.getRawRequest() += buf;

	if (this->request.tryMakeRequest() == true)
	{
		this->status = REQUEST_COMPLETE;
		std::cout << "method:" << this->request.getMethod() << std::endl;
		std::cout << "uri:" << this->request.getUri() << std::endl;

		// this->request.initRequest(); // init은 Response에서 하는걸로 합시당!
	}
	// std::cout << "Raw Body: |" << this->request.getRawBody() << "|" << std::endl;
	// std::cout << "Client status: |" << this->status << "|" << std::endl;
	// std::cout << "Raw Request: |" << this->request.getRawRequest() << "|" << std::endl;
	// std::cout << "Temp Body: |" << this->request.getTempBody() << "|" << std::endl;
	// std::cout << std::endl;

	return (1);
	
}
