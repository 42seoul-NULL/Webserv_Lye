#include "Server.hpp"
#include "Manager.hpp"
#include "Location.hpp"
#include "Client.hpp"
#include "CGI.hpp"
#include "utils.hpp"
#include <unistd.h>
#include <dirent.h>

Server::Server() : port(-1), socket_fd(-1)
{
	this->session_count = 0;
}

Server::Server(const Server& src)
{
	this->ip = src.ip;
	this->port = src.port;
	this->server_name =	src.server_name;
	this->socket_fd = src.socket_fd;
	this->locations = src.locations;
	this->clients = src.clients;
	this->session_count = src.session_count;
}

Server &Server::operator=(const Server &src)
{
	this->ip = src.ip;
	this->port	=	src.port;
	this->server_name	=	src.server_name;
	this->socket_fd		=	src.socket_fd;
	this->locations = src.locations;
	this->clients = src.clients;
	this->session_count = src.session_count;
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

std::map<size_t, std::list<std::string> > &Server::getSessionLogs()
{
	return (this->session_logs);
}


std::map<std::string, Location> &Server::getLocations()
{
	return (this->locations);
}

int Server::acceptClient(int server_fd)
{
	struct sockaddr_in  client_addr;
	socklen_t			addr_size = sizeof(client_addr);

	std::cout << "\033[32m server connection called \033[0m" << std::endl;	
	int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
	if (client_socket == -1)
	{
		std::cerr << "failed to connect client" << std::endl;
		return (-1);
	}
	
	fcntl(client_socket, F_SETFL, O_NONBLOCK);

	this->clients[client_socket].setServerSocketFd(server_fd);
	this->clients[client_socket].setSocketFd(client_socket);
	this->clients[client_socket].setStatus(REQUEST_RECEIVING);
	this->clients[client_socket].setServer(*this);

	//fd_table 세팅
	FDType *client_fdtype = new ClientFD(CLIENT_FDTYPE, &this->clients[client_socket]);
	setFDonTable(client_socket, FD_RDWR, client_fdtype);

	std::cout << "connected client : " << client_socket << std::endl;
	return (client_socket);
}

bool Server::isCgiRequest(Location &location, Request &request)
{
	std::map<std::string, std::string> &cgi_infos = location.getCgiInfos();

	size_t dot_pos = request.getUri().find('.');
	if (dot_pos == std::string::npos)
		return (false);
	size_t ext_end = dot_pos;
	while (ext_end != request.getUri().length() && request.getUri()[ext_end] != '/' && request.getUri()[ext_end] != '?')
		ext_end++;
	
	std::string res = request.getUri().substr(dot_pos, ext_end - dot_pos);
	
	std::map<std::string, std::string>::const_iterator found;
	if ((found = cgi_infos.find(res)) == cgi_infos.end())
		return (false);
	
	while (request.getUri()[dot_pos] != '/')
		dot_pos--;
	res = request.getUri().substr(dot_pos + 1, ext_end - dot_pos - 1);


	CGI	cgi;
	cgi.testCGICall(request, location, res, found->second);
	return (true);
}

int    Server::createFileWithDirectory(std::string path)
{
    size_t  pos;
    size_t  n;
    int     fd;

    n = 0;
    pos = path.find("/", n);
    while (pos != std::string::npos)
    {
        std::string temp = path.substr(0, pos);
        mkdir(temp.c_str(), 0755);
        n = pos + 1;
        pos = path.find("/", n);
    }
    fd = open(path.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0755);
	return (fd);
}

bool Server::isCorrectAuth(Location &location, Client &client)
{
	char auth_key[200];

	memset(auth_key, 0, 200);
	
	std::multimap<std::string, std::string>::iterator iter = client.getRequest().getHeaders().find("Authorization");

	std::size_t found = iter->second.find(' ');
	MANAGER->decode_base64(iter->second.substr(found + 1).c_str(), auth_key, iter->second.length());
	if (std::string(auth_key) != location.getAuthKey())
		return (false);
	return (true);
}

bool Server::isDirectoryName(const std::string &path)
{
	struct stat sb;
	if (stat(path.c_str(), &sb) == -1)
		return (false);
	return (true);
}

int Server::cleanUpLocationRoot(Client &client, const std::string &root, Location &location)
{
	std::string path = root;
	DIR *dir_ptr;
	struct dirent *file;
	if ((dir_ptr = opendir(path.c_str())) == NULL)
	{
		std::cerr << "opendir() error!" << std::endl;
		client.getResponse().makeErrorResponse(500, &location);
		return (500);
	}
	if (path[path.length() - 1] != '/')
		path += '/';
	std::string name;
	int ret;
	while ((file = readdir(dir_ptr)) != NULL)
	{
		name = std::string(file->d_name);
		if (name == "." || name == "..")
			continue ;
		if (file->d_type == DT_DIR)
		{
			ret = ft_remove_directory(path + name);
			if (ret == 1)
			{
				client.getResponse().makeErrorResponse(500, &location);
				return (500);
			}
		}
		else
			unlink((path + name).c_str());
	}
	return (200);
}

size_t Server::generateNewSession(void)
{
	size_t ret = this->session_count;
	++this->session_count;
	return (ret);
}
