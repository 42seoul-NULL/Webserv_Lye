#include "../includes/Server.hpp"
#include "Manager.hpp"
#include "Location.hpp"
#include "Client.hpp"

Server::Server() : port(-1)
{
	
}

Server::Server(const Server& src)
{
	this->ip = src.ip;
	this->port = src.port;
	this->server_name =	src.server_name;
	this->socket_fd = src.socket_fd;
	this->locations.insert(src.locations.begin(), src.locations.end());
}

Server &Server::operator=(const Server &src)
{
	this->ip = src.ip;
	this->port	=	src.port;
	this->server_name	=	src.server_name;
	this->socket_fd		=	src.socket_fd;
	this->locations.clear();
	this->locations.insert(src.locations.begin(), src.locations.end());

	return (*this);
}

Server::~Server()
{
	return ;
}

void	Server::setPort(unsigned short port)
{
	this->port = port;
	return ;
}

void	Server::setIP(const std::string &ip)
{
	this->ip = ip;
	return ;
}

void	Server::setServerName(const std::string &server_name)
{
	this->server_name = server_name;
	return ;
}

void	Server::setSocketFd(int socket_fd)
{
	this->socket_fd = socket_fd;
	return ;
}

const std::string &Server::getIP() const
{
	return (this->ip);
}

const std::string &Server::getServerName() const
{
	return (this->server_name);
}

unsigned short		Server::getPort() const
{
	return (this->port);
}

int					Server::getSocketFd() const
{
	return (this->socket_fd);
}

std::map<int, Client> &Server::getClients()
{
	return (this->clients);
}


std::map<std::string, Location> &Server::getLocations()
{
	return (this->locations);
}

int Server::acceptClient(int server_fd, int &fd_max)
{
	struct sockaddr_in  client_addr;
	socklen_t			addr_size = sizeof(client_addr);

	std::cout << "\033[32m server connection called \033[0m" << std::endl;	
	int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
	fcntl(client_socket, F_SETFL, O_NONBLOCK);

	if (fd_max < client_socket)
		fd_max = client_socket;

	this->clients[client_socket].setServerSocketFd(server_fd);
	this->clients[client_socket].setSocketFd(client_socket);
	this->clients[client_socket].setLastRequestMs(ft_get_time());
	this->clients[client_socket].setStatus(REQUEST_RECEIVING);
	this->clients[client_socket].setServer(*this);

	FDType *client_fdtype = new ClientFD(CLIENT_FDTYPE, &this->clients[client_socket]);
	Manager::getInstance()->getFDTable().insert(std::pair<int, FDType*>(client_socket, client_fdtype));

	std::cout << "connected client : " << client_socket << std::endl;
	return (client_socket);
}

bool Server::isCgiRequest(Location &location, Request &request)
{
	std::vector<std::string> &cgi_extensions = location.getCgiExtensions();

	// uri에서 cgi extension 파싱하여 location의 그것과 매칭되는지 확인
	// 매칭되면 cgi 처리, 아니면 일단 response 만들러 ㄱㄱ

	size_t dot_pos = request.getUri().find('.');
	if (dot_pos == std::string::npos)
		return (false);

	size_t ext_end = dot_pos;
	while (ext_end != request.getUri().length() && request.getUri()[ext_end] != '/' && request.getUri()[ext_end] != '?')
		ext_end++;
	
	std::string res = request.getUri().substr(dot_pos, ext_end - dot_pos);
	if (std::find(cgi_extensions.begin(), cgi_extensions.end(), res) == cgi_extensions.end())
		return (false);
	return (true);
}


//for test
void		Server::show()
{
	std::cout << "ip	:	" << this->ip << std::endl;
	std::cout << "port	:	" << this->port << std::endl;
	std::cout << "server_name	:	" << this->server_name << std::endl;
	std::cout << "============= location start =============" << std::endl;
	for (std::map<std::string, Location>::iterator iter = locations.begin(); iter != locations.end(); iter++)
	{
		std::cout << "=== Location Key : " << iter->first << " ===" << std::endl;
		iter->second.show();
	}
	std::cout << "============= location end ===============" << std::endl;
}