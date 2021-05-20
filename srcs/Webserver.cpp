#include "../includes/Webserver.hpp"

Webserver::Webserver() : fd_max(-1)
{
	FT_FD_ZERO(&(this->reads));
	FT_FD_ZERO(&(this->writes));
	FT_FD_ZERO(&(this->errors));
}

Webserver::~Webserver()
{

}

Webserver::Webserver(const Webserver &src)
{
	this->reads = src.reads;
	this->writes = src.writes;
	this->errors = src.errors;
	this->fd_max = src.fd_max;
	this->servers.insert(src.servers.begin(), src.servers.end());
}

Webserver& Webserver::operator=(const Webserver &src)
{
	this->reads = src.reads;
	this->writes = src.writes;
	this->errors = src.errors;
	this->fd_max = src.fd_max;
	this->servers.clear();
	this->servers.insert(src.servers.begin(), src.servers.end());
	return (*this);
}

void	Webserver::disconnect_client(Client &client)
{
	int client_socket_fd = client.getSocketFd();

	FT_FD_CLR(client_socket_fd, &(this->reads));
	FT_FD_CLR(client_socket_fd, &(this->writes));
	FT_FD_CLR(client_socket_fd, &(this->errors));
	close(client_socket_fd);
	
	Server *server = client.getServer();
	server->getClients().erase(client_socket_fd);

	delete Manager::getInstance()->getFDTable()[client_socket_fd];
	Manager::getInstance()->getFDTable()[client_socket_fd] = NULL;

	return ;
}

const Server &Webserver::getServerFromClient(int server_socket_fd, const std::string &server_name)
{
	if (this->servers[server_socket_fd].find(server_name) == this->servers[server_socket_fd].end())
		return (this->servers[server_socket_fd].begin()->second);
	return ( (this->servers[server_socket_fd][server_name]) );
}

Location &Webserver::getPerfectLocation(int server_socket_fd, const std::string &server_name, const std::string &uri)
{
	std::map<std::string, Location> *locs;

	if (this->servers[server_socket_fd].find(server_name) == this->servers[server_socket_fd].end()) // 못찾으면
		locs = &(this->servers[server_socket_fd].begin()->second.getLocations());
	else
		locs = &(this->servers[server_socket_fd][server_name].getLocations());

	Location &ret = (*locs)["/"];

	std::string key = "";
	for (std::string::const_iterator iter = uri.begin(); iter != uri.end(); iter++)
	{
		key += *iter;
		if (*iter == '/')
		{
			if (locs->find(key) == locs->end())
				return (ret);
			else
				ret = (*locs)[key];
		}
	}
	return (ret);
}

bool	Webserver::initServers(int queue_size)
{
	for (std::map<int, Server>::iterator iter = Manager::getInstance()->getServers().begin(); iter != Manager::getInstance()->getServers().end(); iter++)
	{
		struct sockaddr_in  server_addr;

		iter->second.setSocketFd(socket(PF_INET, SOCK_STREAM, 0));

		FDType *new_socket_fd = new ServerFD(SERVER_FDTYPE);
		Manager::getInstance()->getFDTable().insert(std::pair<int, FDType*>(iter->second.getSocketFd(), new_socket_fd));
		
		int option = 1;
		setsockopt(iter->second.getSocketFd(), SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));

		ft_memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = inet_addr(iter->second.getIP().c_str());
		server_addr.sin_port = htons(iter->second.getPort());

		if (bind(iter->second.getSocketFd(), (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
		{
			std::cerr << "bind() error" << std::endl;
			throw strerror(errno);
		}
		if (listen(iter->second.getSocketFd(), queue_size) == -1)
			throw strerror(errno);

		std::cout << "Server " << iter->second.getServerName() << "(" << iter->second.getIP() << ":" << iter->second.getPort() << ") started" << std::endl;

		// 서버소켓은 read 와 error 만 검사.
		FT_FD_SET(iter->second.getSocketFd(), &(this->reads));
		FT_FD_SET(iter->second.getSocketFd(), &(this->errors));

		
		this->servers[iter->second.getSocketFd()] = iter->second;

		if (this->fd_max < iter->second.getSocketFd())
			this->fd_max = iter->second.getSocketFd();
	}
	return (true);
}

bool	Webserver::run(struct timeval	timeout, unsigned int buffer_size)
{
	fd_set	cpy_reads;
	fd_set	cpy_writes;
	fd_set	cpy_errors;

	int fd_num;

	while (1)
	{
		usleep(5); // cpu 점유가 100% 까지 올라가는 것을 막기 위해서

		cpy_reads = this->reads;
		cpy_writes = this->writes;
		cpy_errors = this->errors;

		if ( (fd_num = select(this->fd_max + 1, &cpy_reads, &cpy_writes, &cpy_errors, &timeout)) == -1)
			throw strerror(errno);

		if (fd_num == 0)
			continue ;
		
		FDType *fd = NULL;
		for (int i = 0; i <= this->fd_max; i++)
		{
			fd = Manager::getInstance()->getFDTable()[i];
			if (fd == NULL)
				continue ;
			
			//if  (ft_get_time() - ) // 타임아웃 체크
					// 클라이언트 timeout disconnect

			if (FT_FD_ISSET(i, &cpy_reads))
			{
				if (fd->getType() == SERVER_FDTYPE)
				{
					int client_socket_fd = this->servers[i].acceptClient(i, this->fd_max);
					FT_FD_SET(client_socket_fd, &(this->reads));
					FT_FD_SET(client_socket_fd, &(this->writes));
					FT_FD_SET(client_socket_fd, &(this->errors));
				}
				else if (fd->getType() == CLIENT_FDTYPE)
				{
					ClientFD *client_fd = dynamic_cast<ClientFD *>(fd);
					if (client_fd->to_client->readRequest() == DISCONNECT_CLIENT)
						disconnect_client(*(client_fd->to_client));

					if (client_fd->to_client->getStatus() == REQUEST_COMPLETE)
					{
						// response
						
						// uri 확장자 체크 -> cgi call -> makeResponse()
						// makeResponse()
					}
				}
				else if (fd->getType() == RESOURCE_FDTYPE)
				{
					// resource read
				}
			}
			else if (FT_FD_ISSET(i, &cpy_writes))
			{
				

				if (fd->getType() == CLIENT_FDTYPE)
				{
					// 클라이언트 Response write
				}
				else if (fd->getType() == RESOURCE_FDTYPE)
				{
					// resource write
				}
			}
			else if (FT_FD_ISSET(i, &cpy_errors))
			{
				if (fd->getType() == SERVER_FDTYPE)
				{
					// 서버 에러 - 프로그램 터짐
				}
				else if (fd->getType() == CLIENT_FDTYPE)
				{
					// 클라이언트 에러 - disconnect
				}
				else if (fd->getType() == RESOURCE_FDTYPE)
				{
					// 리소스 io error - internal server error
				}
			}
		}
	}
	return (true);
}
