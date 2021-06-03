#include "Manager.hpp"
#include "Webserver.hpp"
#include "libft.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Location.hpp"
#include "CGI.hpp"
#include <dirent.h>

#define GENERAL_RESPONSE 0
#define CGI_RESPONSE 1
#define REDIRECT_RESPONSE 2
#define AUTOINDEX_RESPONSE 3

Webserver::Webserver() : fd_max(-1)
{
	
}

Webserver::~Webserver()
{

}

Webserver::Webserver(const Webserver &src)
{
	this->fd_max = src.fd_max;
	this->servers.insert(src.servers.begin(), src.servers.end());
}

Webserver& Webserver::operator=(const Webserver &src)
{
	this->fd_max = src.fd_max;
	this->servers.clear();
	this->servers.insert(src.servers.begin(), src.servers.end());
	return (*this);
}

void	Webserver::setFDMax(int fd_max)
{
	this->fd_max = fd_max;
}

int		Webserver::getFDMax(void)
{
	return (this->fd_max);
}

void	Webserver::disconnect_client(Client &client)
{
	int client_socket_fd = client.getSocketFd();

	Server *server = client.getServer();
	Client *client_pointer = &client;
	server->getClients().erase(client_socket_fd);

	ResourceFD *resource_fdtype = NULL;
	PipeFD *pipe_fdtype = NULL;
	std::map<int, FDType*>::iterator temp;
	for (std::map<int, FDType*>::iterator iter = MANAGER->getFDTable().begin(); iter != MANAGER->getFDTable().end(); )
	{
		if ((resource_fdtype = dynamic_cast<ResourceFD *>(iter->second)))
		{
			if (resource_fdtype->getClient() == client_pointer)
			{
				clrFDonTable(iter->first, FD_RDWR);
				close(iter->first);
				delete resource_fdtype;
				temp = iter;
				temp++;
				MANAGER->getFDTable().erase(iter->first);
				iter = temp;
				continue;
			}
		}
		else if ((pipe_fdtype = dynamic_cast<PipeFD *>(iter->second)))
		{
			if (pipe_fdtype->getClient() == client_pointer)
			{
				clrFDonTable(iter->first, FD_RDWR);
				close(iter->first);
				delete pipe_fdtype;
				temp = iter;
				temp++;
				MANAGER->getFDTable().erase(iter->first);
				iter = temp;
				continue;
			}
		}
		iter++;
	}

	MANAGER->deleteFromFDTable(client_socket_fd, MANAGER->getFDTable()[client_socket_fd], FD_RDWR);
}

bool	Webserver::initServers(int queue_size)
{
	FT_FD_ZERO(&(MANAGER->getReads()));
	FT_FD_ZERO(&(MANAGER->getWrites()));
	FT_FD_ZERO(&(MANAGER->getErrors()));

	for (std::map<int, Server>::iterator iter = MANAGER->getServerConfigs().begin(); iter != MANAGER->getServerConfigs().end(); iter++)
	{
		struct sockaddr_in  server_addr;

		iter->second.setSocketFd(socket(PF_INET, SOCK_STREAM, 0));
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
		this->servers[iter->second.getSocketFd()] = iter->second;

		// FDTable에 insert
		FDType *new_socket_fdtype = new ServerFD(SERVER_FDTYPE);
		MANAGER->getFDTable().insert(std::pair<int, FDType*>(iter->second.getSocketFd(), new_socket_fdtype));
		// 서버소켓은 read 와 error 만 검사.
		setFDonTable(iter->second.getSocketFd(), FD_RDONLY);
		if (this->fd_max < iter->second.getSocketFd())
			this->fd_max = iter->second.getSocketFd();
	}
	return (true);
}

Location &Webserver::getPerfectLocation(Server &server, const std::string &uri)
{
	size_t pos;
	std::string uri_loc = "";

	pos = uri.find('.');
	if (pos != std::string::npos)
	{
		while (uri[pos] != '/')
			pos--;
		uri_loc = uri.substr(0, pos + 1);
	}
	else
	{
		pos = uri.find('?');
		if (pos != std::string::npos)
			uri_loc = uri.substr(0, pos);
		else
			uri_loc = uri;
	}
	if (uri_loc[uri_loc.length() - 1] != '/')
		uri_loc += "/";

	std::map<std::string, Location> &loc_map = server.getLocations();
	Location *ret = &loc_map["/"];

	std::string key = "";
	for (std::string::const_iterator iter = uri_loc.begin(); iter != uri_loc.end(); iter++)
	{
		key += *iter;
		if (*iter == '/')
		{
			if (loc_map.find(key) == loc_map.end())
				return (*ret);
			else
				ret = &loc_map[key];
		}
	}
	return (*ret);
}

bool	Webserver::run(struct timeval timeout)
{
	fd_set	cpy_reads;
	fd_set	cpy_writes;
	fd_set	cpy_errors;

	int fd_num;

	while (1)
	{
		usleep(5); // cpu 점유가 100% 까지 올라가는 것을 막기 위해서

		cpy_reads = MANAGER->getReads();
		cpy_writes = MANAGER->getWrites();
		cpy_errors = MANAGER->getErrors();

		if ( (fd_num = select(this->fd_max + 1, &cpy_reads, &cpy_writes, &cpy_errors, &timeout)) == -1)
			throw strerror(errno);

		if (fd_num == 0)
			continue ;
		
		FDType *fd = NULL;
		for (int i = 0; i <= this->fd_max; i++)
		{	
			if (MANAGER->getFDTable().count(i) == 0) // fd_table에 없는 fd라면 continue
				continue ;
			fd = MANAGER->getFDTable()[i];

			if (FT_FD_ISSET(i, &cpy_reads))
			{
				if (fd->getType() == SERVER_FDTYPE)
				{
					int client_socket_fd = this->servers[i].acceptClient(i, this->fd_max);
					setFDonTable(client_socket_fd, FD_RDWR);
					if (this->fd_max < client_socket_fd)
						this->fd_max = client_socket_fd;
				}
				else if (fd->getType() == CLIENT_FDTYPE)
				{
					Client *client = dynamic_cast<ClientFD *>(fd)->getClient();

					if (client->readRequest() == DISCONNECT_CLIENT)
					{
						disconnect_client(*client);
						std::cout << "disconnected: " << i << std::endl;
						continue ;
					}
					if (client->getStatus() == REQUEST_COMPLETE)
						this->prepareResponse(*client);
				}
				else if (fd->getType() == RESOURCE_FDTYPE || fd->getType() == CGI_RESOURCE_FDTYPE)
				{
					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd);
					resource_fd->getClient()->getResponse().tryMakeResponse(resource_fd, i, resource_fd->getClient()->getRequest());
				}
				else if (fd->getType() == ERROR_RESOURCE_FDTYPE)
				{
					char buf[BUFFER_SIZE + 1];
					int read_size;
					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd);

					read_size = read(i, buf, BUFFER_SIZE);
					if (read_size == -1)
					{
						std::cerr << "temporary resource read error!" << std::endl;
						continue ;
					}
					buf[read_size] = 0;
					Client *client = resource_fd->getClient();
					client->getResponse().getBody().append(buf);
					if (read_size < BUFFER_SIZE)
					{
						client->getResponse().getHeaders().insert(std::pair<std::string, std::string>("Content-Length", ft_itoa(client->getResponse().getBody().length())));
						client->getResponse().makeStartLine();
						client->getResponse().makeRawResponse();
						client->setStatus(RESPONSE_COMPLETE);
						MANAGER->deleteFromFDTable(i, fd, FD_RDONLY);
					}
				}
			}
			else if (FT_FD_ISSET(i, &cpy_writes))
			{
				if (fd->getType() == CLIENT_FDTYPE)
				{
					// 클라이언트 Response write
					// 다른 곳에서 응답 raw_data 다 준비해놓고 여기서는 write 및 clear()만
					Client *client = dynamic_cast<ClientFD *>(fd)->getClient();

					if (client->getStatus() == RESPONSE_COMPLETE)
					{
						size_t res_idx = client->getResponse().getResIdx();
						int write_size = write(i, client->getResponse().getRawResponse().c_str() + res_idx, client->getResponse().getRawResponse().length() - res_idx);
						if (write_size == -1)
						{
							std::cerr << "temporary client write error!" << std::endl;
							continue ;
						}
						client->getResponse().setResIdx(res_idx + write_size);
						if (client->getResponse().getResIdx() >= client->getResponse().getRawResponse().length())
						{
							client->getRequest().initRequest();
							client->getResponse().initResponse();
							client->setStatus(REQUEST_RECEIVING);
						}
					}
				}
				else if (fd->getType() == RESOURCE_FDTYPE)
				{
					// resource write - PUT
					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd);

					size_t write_idx = resource_fd->getWriteIdx();
					int write_size = write(i, resource_fd->getData().c_str() + write_idx, resource_fd->getData().length() - write_idx);
					if (write_size == -1)
					{
						std::cerr << "temporary resource write error!" << std::endl;
						continue ;
					}
					resource_fd->setWriteIdx(write_idx + write_size);
					if (resource_fd->getWriteIdx() >= resource_fd->getData().length())
					{
						resource_fd->getClient()->getResponse().makePutResponse(resource_fd->getClient()->getRequest());
						MANAGER->deleteFromFDTable(i, fd, FD_WRONLY);
					}
				}
				else if (fd->getType() == PIPE_FDTYPE)
				{
					// cgi pipe에 body write
					PipeFD *pipefd = dynamic_cast<PipeFD *>(fd);

					int write_idx = pipefd->getWriteIdx();

					if (pipefd->getData().length() - write_idx > BUFFER_SIZE)
					{
						int write_size = write(i, pipefd->getData().c_str() + write_idx, BUFFER_SIZE);
						if (write_size == -1)
						{
							std::cerr << "temporary client write error!" << std::endl;
							continue ;
						}
						pipefd->setWriteIdx(write_idx + write_size);
						continue ;
					}
					else
					{
						int write_size = write(i, pipefd->getData().c_str() + write_idx, pipefd->getData().length() - write_idx);
						if (write_size == -1)
						{
							std::cerr << "temporary client write error!" << std::endl;
							continue ;
						}
						if (static_cast<size_t>(write_size) < pipefd->getData().length() - write_idx)
						{
							pipefd->setWriteIdx(write_idx + write_size);
							continue ;
						}
						close(pipefd->getFdRead());
						MANAGER->deleteFromFDTable(i, fd, FD_WRONLY);
					}
				}
			}
			else if (FT_FD_ISSET(i, &cpy_errors))
			{
				if (fd->getType() == SERVER_FDTYPE)
				{
					// 서버 에러 - 프로그램 터짐
					throw "Server Error!";
				}
				else if (fd->getType() == CLIENT_FDTYPE) // 클라이언트 에러 - 연결 해제
				{
					ClientFD *client_fd = dynamic_cast<ClientFD *>(fd);
					disconnect_client(*(client_fd->getClient()));
					std::cerr << "client error!" << std::endl;
					close(i);
				}
				else if (fd->getType() == RESOURCE_FDTYPE)
				{
					std::cerr << "resource error!" << std::endl;
					Client *client = dynamic_cast<ResourceFD*>(fd)->getClient();

					client->getResponse().makeErrorResponse(500, NULL);
					MANAGER->deleteFromFDTable(i, fd, FD_RDWR);
				}
				else if (fd->getType() == PIPE_FDTYPE)
				{
					Client *client = dynamic_cast<ResourceFD*>(fd)->getClient();
					std::cerr << "pipe error!" << std::endl;

					client->getResponse().makeErrorResponse(500, NULL);
					MANAGER->deleteFromFDTable(i, fd, FD_WRONLY);
				}
			}
		}
	}
	return (true);
}

int Webserver::prepareResponse(Client &client)
{
	Location &location = this->getPerfectLocation( *client.getServer(), client.getRequest().getUri() );

	// auth 체크 - 401
	if (location.getAuthKey() != ""
		&& (client.getRequest().getHeaders().count("Authorization") == 0
		|| !client.getServer()->isCorrectAuth(location, client)))
	{
		client.getResponse().makeErrorResponse(401, &location);
		return (401);
	}

	// Allowed Method인지 체크 - 405
	if (std::find(location.getAllowMethods().begin(), location.getAllowMethods().end(), client.getRequest().getMethod()) == location.getAllowMethods().end())
	{
		client.getResponse().makeErrorResponse(405, &location);
		return (405);
	}
	
	// client_max_body_size 체크
	if (client.getRequest().getRawBody().length() > static_cast<size_t>(location.getRequestMaxBodySize()))
	{
		client.getResponse().makeErrorResponse(413, &location);
		return (413);
	}

	//리다이렉션 체크
	if (location.getRedirectReturn() != -1)
	{
		client.getResponse().makeRedirectResponse(location);
		return (REDIRECT_RESPONSE);
	}

	// response
	if (client.getServer()->isCgiRequest(location, client.getRequest()))
		return (CGI_RESPONSE);
	else
		return (this->prepareGeneralResponse(client, location));
}

int Webserver::prepareGeneralResponse(Client &client, Location &location)
{
	std::string uri = client.getRequest().getUri().substr(0, client.getRequest().getUri().find('?'));

	if (client.getRequest().getMethod() == "GET" || client.getRequest().getMethod() == "HEAD" || client.getRequest().getMethod() == "POST")
	{
		if (uri[uri.length() - 1] != '/')
			uri += '/';
		
		std::string path;
		if (uri == "/")
			path = location.getRoot();
		else
			path = location.getRoot() + uri.substr(location.getLocationName().length());

		struct stat sb;
		if (stat(path.c_str(), &sb) == -1)
		{
			path.erase(--(path.end())); // '/' 떼보고 stat 해봐서
			if (stat(path.c_str(), &sb) == -1) // 없으면 404
			{
				client.getResponse().makeErrorResponse(404, &location);
				return (404);		
			}
		} // if 안들어가면 디렉토리

		if (path[path.length() - 1] == '/') // 디렉토리
		{
			std::string res = location.checkAutoIndex(path);
			if (res == "404")
			{
				client.getResponse().makeErrorResponse(404, &location);
				return (404) ;
			}
			else if (res == "Index of") //autoindex list up
			{
				client.getResponse().makeAutoIndexResponse(path, client.getRequest().getUri());
				return (AUTOINDEX_RESPONSE) ;
			}
			else
				path = res;
		} // 파일은 무조건 존재

		client.getRequest().setPath(path);
		
		int get_fd = open(path.c_str(), O_RDONLY);
		ResourceFD *file_fd = new ResourceFD(RESOURCE_FDTYPE, &client);
		MANAGER->getFDTable().insert(std::pair<int, FDType*>(get_fd, file_fd));
		setFDonTable(get_fd, FD_RDONLY);
		if (this->fd_max < get_fd)
			this->fd_max = get_fd;
	}
	else if (client.getRequest().getMethod() == "PUT")
	{
		bool is_no_slash = false;
		if (uri[uri.length() - 1] != '/')
		{
			uri += '/';
			is_no_slash = true;
		}

		std::string path;
		if (uri == "/")
			path = location.getRoot();
		else
			path = location.getRoot() + uri.substr(location.getLocationName().length());

		if (is_no_slash == true)
			path.erase(--(path.end()));
		struct stat sb;
		stat(path.c_str(), &sb);

		if (path[path.length() - 1] == '/' || S_ISDIR(sb.st_mode))
		{
			client.getResponse().makeErrorResponse(400, &location);
			return (400);
		}
		client.getRequest().setPath(path);
		int put_fd = client.getServer()->createFileWithDirectory(path);
		ResourceFD *file_fd = new ResourceFD(RESOURCE_FDTYPE, &client, client.getRequest().getRawBody());
		MANAGER->getFDTable().insert(std::pair<int, FDType*>(put_fd, file_fd));
		setFDonTable(put_fd, FD_WRONLY);
		if (this->fd_max < put_fd)
			this->fd_max = put_fd;
	}
	else if (client.getRequest().getMethod() == "DELETE")
	{
		if (uri[uri.length() - 1] != '/')
			uri += '/';
		if (uri == location.getLocationName())
		{
			client.getRequest().setPath(location.getRoot());
			client.getServer()->cleanUpLocationRoot(client, location.getRoot());
			client.getResponse().makeDeleteResponse(client.getRequest());
			return (GENERAL_RESPONSE);
		}

		std::string path = location.getRoot() + uri.substr(location.getLocationName().length());
		if (client.getServer()->isDirectoryName(path))
		{
			client.getRequest().setPath(path);
			path.erase(--(path.end()));
			if (ft_remove_directory(path.c_str()) == 1)
			{
				client.getResponse().makeErrorResponse(500, NULL);
				return (500);
			}
		}
		else
		{
			path.erase(--(path.end()));
			client.getRequest().setPath(path);
			unlink(path.c_str());	
		}
		client.getResponse().makeDeleteResponse(client.getRequest());
	}
	return (GENERAL_RESPONSE);
}