#include "Client.hpp"
#include "Manager.hpp"
#include <list>

Client::Client()
{
	this->server_socket_fd = -1;
	this->socket_fd = -1;
	this->status = REQUEST_RECEIVING;
	this->request.setClient(this);
	this->response.setClient(this);
	this->server = NULL;
	this->session_id = 0;
	this->is_new_session = true;
}

Client::Client(int server_socket_fd, int socket_fd) : server_socket_fd(server_socket_fd), socket_fd(socket_fd)
{
	this->status = REQUEST_RECEIVING;
	this->request.setClient(this);
	this->response.setClient(this);
	this->server = NULL;
	this->session_id = 0;
	this->is_new_session = true;
}

Client::~Client() {}

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

void	Client::setSessionFlag(bool flag)
{
	this->is_new_session = flag;
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

Server		*Client::getServer()
{
	return (this->server);
}

size_t	Client::getSessionId(void)
{
	return (this->session_id);
}

bool	Client::getSessionFlag(void)
{
	return (this->is_new_session);
}


int Client::readRequest(void)
{
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
		#ifdef BONUS
			if (this->parseSessionId() == false) // cookie에 session id 적혀있지 않은 경우
			{
				this->session_id = this->server->generateNewSession(); // session id 발급
				this->is_new_session = true;
			}
			else
				this->is_new_session = false;
			this->server->getSessionLogs()[this->session_id].push_back(this->request.getMethod() + " " + this->request.getUri());
		#endif

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
				this->session_id = static_cast<size_t>(atoi(id.c_str()));
				return (true);
			}
			tokens.clear();
		}
		++iter_first;
	}
	return (false);
}
