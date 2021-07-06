#include "../includes/Client.hpp"
#include "Manager.hpp"

Client::Client()
{
	this->server_socket_fd = -1;
	this->socket_fd = -1;
	this->status = REQUEST_RECEIVING;
	this->request.setClient(this);
	this->response.setClient(this);
	this->last_request_ms = 0;
	this->server = NULL;
	this->session_id = 0;
}

Client::Client(int server_socket_fd, int socket_fd) : server_socket_fd(server_socket_fd), socket_fd(socket_fd)
{
	this->status = REQUEST_RECEIVING;
	this->request.setClient(this);
	this->response.setClient(this);
	this->last_request_ms = 0;
	this->server = NULL;
	this->session_id = 0;
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

void	Client::setSessionId(size_t session_id)
{
	this->session_id = session_id;
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

size_t	Client::getSessionId(void)
{
	return (this->session_id);
}


int Client::readRequest(void)
{
	this->setLastRequestMs(ft_get_time());
	char buf[BUFFER_SIZE];

	int readed;
	readed = read(this->socket_fd, buf, BUFFER_SIZE - 1);
	if (readed <= 0)
	{
		if (readed == 0)
			return (DISCONNECT_CLIENT);
		else
		{
			std::cerr << "client read error!" << std::endl;
			return (1);
		}
	}
	buf[readed] = 0;

	this->request.getRawRequest() += buf;

	if (this->request.tryMakeRequest() == true)
	{
		if (this->parseSessionId() == false)
			this->session_id = this->server->generateNewSession();
		this->status = REQUEST_COMPLETE;
	}
	return (1);
}

bool Client::parseSessionId(void)
{
	if (this->request.getHeaders().count("Cookie") == 0)
		return (false);

	std::multimap<std::string, std::string>::const_iterator iter_first = this->request.getHeaders().lower_bound("Cookie");
	std::multimap<std::string, std::string>::const_iterator iter_last = this->request.getHeaders().upper_bound("Cookie");

	size_t pos;
	while (iter_first != iter_last)
	{
		if ((pos = iter_first->second.find("webserv_session_id")) != std::string::npos)
		{
			std::vector<std::string> tokens;
			ft_split(iter_first->second, "; ", tokens);

			size_t id_pos;
			for (std::vector<std::string>::const_iterator iter = tokens.begin(); iter != tokens.end(); ++iter)
			{
				id_pos = (*iter).find("webserv_session_id=");
				if (id_pos == std::string::npos)
					break ;
				std::string id = (*iter).substr(id_pos + 19, (*iter).find(';', id_pos));
				this->session_id = static_cast<size_t>(ft_atoi(id));
				return (true);
			}
			tokens.clear();
		}
	}
	return (false);
}
