#include "Manager.hpp"
#include "Webserver.hpp"
#include "../libft_cpp/libft.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Location.hpp"
#include "CGI.hpp"

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

	clrFDonTable(client_socket_fd, FD_RDWR);
	close(client_socket_fd);
	
	Server *server = client.getServer();
	Client *client_pointer = &client;
	server->getClients().erase(client_socket_fd);


	ResourceFD *resource_fdtype = NULL;
	PipeFD *pipe_fdtype = NULL;
	for (std::map<int, FDType*>::iterator iter = MANAGER->getFDTable().begin(); iter != MANAGER->getFDTable().end(); )
	{
		if ((resource_fdtype = dynamic_cast<ResourceFD *>(iter->second)))
		{
			if (resource_fdtype->to_client == client_pointer)
			{
				clrFDonTable(iter->first, FD_RDWR);
				delete resource_fdtype;
				MANAGER->getFDTable().erase(iter->first);
			}
			else
				iter++;
		}
		else if ((pipe_fdtype = dynamic_cast<PipeFD *>(iter->second)))
		{
			if (pipe_fdtype->to_client == client_pointer)
			{
				clrFDonTable(iter->first, FD_RDWR);
				delete pipe_fdtype;
				MANAGER->getFDTable().erase(iter->first);
			}
			else
				iter++;
		}
		else
			iter++;
	}

	delete MANAGER->getFDTable()[client_socket_fd];
	MANAGER->getFDTable()[client_socket_fd] = NULL;
	MANAGER->getFDTable().erase(client_socket_fd);

	return ;
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

		// FDTable에 insert
		FDType *new_socket_fdtype = new ServerFD(SERVER_FDTYPE);
		MANAGER->getFDTable().insert(std::pair<int, FDType*>(iter->second.getSocketFd(), new_socket_fdtype));
		
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

		//std::cout << "Server " << iter->second.getServerName() << "(" << iter->second.getIP() << ":" << iter->second.getPort() << ") started" << std::endl;

		// 서버소켓은 read 와 error 만 검사.
		setFDonTable(iter->second.getSocketFd(), FD_RDONLY);
		if (MANAGER->getWebserver().getFDMax() < iter->second.getSocketFd())
		{
			MANAGER->getWebserver().setFDMax(iter->second.getSocketFd());
		}

		
		this->servers[iter->second.getSocketFd()] = iter->second;

		if (this->fd_max < iter->second.getSocketFd())
			this->fd_max = iter->second.getSocketFd();
	}
	return (true);
}

Location &Webserver::getPerfectLocation(Server &server, const std::string &uri)
{
	size_t pos;
	std::string uri_loc = "";

	//std::cout <<"uri: [" << uri << "] uri print in Webserver.cpp 159line" << std::endl;

	pos = uri.find('.');
	if (pos != std::string::npos)
	{
		// 	. 이 있으니까 거기까지 자르면됨
		while (uri[pos] != '/')
			pos--;
		uri_loc = uri.substr(0, pos + 1); // 맨뒤에 / 붙여줄꺼면 +1 하고 아니면 안하고 어떻게할까숑 
	}
	else
	{
		pos = uri.find('?');
		if (pos != std::string::npos)
		{	
			// ? 있으니까 딱 거기까지 잘라버리면 될듯
			uri_loc = uri.substr(0, pos);
		}
		else
		{
			uri_loc = uri;
		}
	}
	if (uri_loc[uri_loc.length() - 1] != '/')
		uri_loc += "/";

	std::map<std::string, Location> &loc_map = server.getLocations();
	Location *ret = &loc_map["/"];

	std::string key = "";
	//std::cout << "[" << uri_loc << "] uri_loc print in Webserver.cpp in 189 line" << std::endl;
	// //std::cout << ret.getLocationName() << std::endl;
	// for (std::map<std::string, Location>::iterator iter = loc_map.begin(); iter != loc_map.end(); iter++)
	// {
	// 	//std::cout << iter->second.getLocationName() << std::endl;
	// 	//std::cout << iter->second.getRoot() << std::endl;
	// }
	for (std::string::const_iterator iter = uri_loc.begin(); iter != uri_loc.end(); iter++)
	{
		key += *iter;
		if (*iter == '/')
		{
			if (loc_map.find(key) == loc_map.end())
			{
				return (*ret);
			}
			else
			{
				ret = &loc_map[key];
			}
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
		// //std::cout << "check select" << std::endl;

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
					{
						this->fd_max = client_socket_fd;
					}
				}
				else if (fd->getType() == CLIENT_FDTYPE)
				{
					Client *client = dynamic_cast<ClientFD *>(fd)->to_client;

					//if  (ft_get_time() - ) // 타임아웃 체크 필요 // 그냥 하지 말자...

					if (client->readRequest() == DISCONNECT_CLIENT)
					{
						disconnect_client(*client);
						//std::cout << "disconnected: " << i << std::endl;
						continue ;
					}
					if (client->getStatus() == REQUEST_COMPLETE)
					{
						this->prepareResponse(*client);
						
					}
				}
				else if (fd->getType() == RESOURCE_FDTYPE || fd->getType() == CGI_RESOURCE_FDTYPE)
				{
					// //std::cout << "try to read resource" << std::endl;
					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd);
					resource_fd->to_client->getResponse().tryMakeResponse(resource_fd, i, resource_fd->to_client->getRequest());
				}
				else if (fd->getType() == ERROR_RESOURCE_FDTYPE)
				{
					//std::cout << "try to read error resource" << std::endl;

					char buf[BUFFER_SIZE + 1];
					int read_size;
					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd);

					read_size = read(i, buf, BUFFER_SIZE);
					buf[read_size] = 0;
					resource_fd->to_client->getResponse().getBody().append(buf);
					if (read_size < BUFFER_SIZE)
					{
						resource_fd->to_client->getResponse().getHeaders().insert(std::pair<std::string, std::string>("Content-Length", ft_itoa(resource_fd->to_client->getResponse().getBody().length())));
						resource_fd->to_client->getResponse().makeStartLine();
						resource_fd->to_client->getResponse().makeRawResponse();
						resource_fd->to_client->setStatus(RESPONSE_COMPLETE);
						delete fd;
						MANAGER->getFDTable().erase(i);
						clrFDonTable(i, FD_RDONLY);
					}
				}
			}
			else if (FT_FD_ISSET(i, &cpy_writes))
			{
				if (fd->getType() == CLIENT_FDTYPE)
				{
					// //std::cout << "select: client write" << std::endl;
					//if  (ft_get_time() - ) // 타임아웃 체크 필요 // 그냥 하지 말자...
					Client *client = dynamic_cast<ClientFD *>(fd)->to_client;
					// 클라이언트 Response write
					// 다른 곳에서 응답 raw_data 다 준비해놓고 여기서는 write 및 clear()만
					// //std::cout << "try to response" << std::endl;
					if (client->getStatus() == RESPONSE_COMPLETE)
					{
						//std::cout << "ready to response" << std::endl;
						// if (client->getResponse().getWriting() == false)
						// 	client->getResponse().setWriting(true);
						int res_idx = client->getResponse().getResIdx();

						int write_size = write(i, client->getResponse().getRawResponse().c_str() + res_idx, client->getResponse().getRawResponse().length() - res_idx);
						client->getResponse().setResIdx(res_idx + write_size);
						if (client->getResponse().getResIdx() >= client->getResponse().getRawResponse().length())
						{
							client->getRequest().initRequest();
							client->getResponse().initResponse();
							client->setStatus(REQUEST_RECEIVING);
						}
						// std::cout << "finished response" << std::endl;
					}
				}
				else if (fd->getType() == RESOURCE_FDTYPE)
				{
					// resource write - PUT

					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd);

					if (resource_fd->getData().length() > BUFFER_SIZE)
					{
						int write_size = write(i, resource_fd->getData().c_str(), BUFFER_SIZE);
						resource_fd->getData().erase(0, write_size);
						continue ;
					}
					else
					{
						// //std::cout << "data: " << resource_fd->getData() << std::endl;
						int write_size = write(i, resource_fd->getData().c_str(), resource_fd->getData().length());
						if (static_cast<size_t>(write_size) < resource_fd->getData().length())
						{
							resource_fd->getData().erase(0, write_size);
							continue ;
						}

						resource_fd->getData().clear();
						//std::cout << "resource writing end" << std::endl;
						resource_fd->to_client->getResponse().tryMakePutResponse(resource_fd->to_client->getRequest());
						delete fd;
						MANAGER->getFDTable()[i] = NULL;
						MANAGER->getFDTable().erase(i);
						clrFDonTable(i, FD_WRONLY);
						close(i);
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
						pipefd->setWriteIdx(write_idx + write_size);
						continue ;
					}
					else
					{
						int write_size = write(i, pipefd->getData().c_str() + write_idx, pipefd->getData().length() - write_idx);
						if (static_cast<size_t>(write_size) < pipefd->getData().length() - write_idx)
						{
							pipefd->setWriteIdx(write_idx + write_size);
							continue ;
						}

						//std::cout << "write cgi pipe end" << std::endl;
						close(pipefd->fd_read);
						close(i);
						delete fd;
						MANAGER->getFDTable()[i] = NULL;
						MANAGER->getFDTable().erase(i);
						clrFDonTable(i, FD_WRONLY);
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
					//std::cout << "jayun error\n";
					ClientFD *client_fd = dynamic_cast<ClientFD *>(fd);
					disconnect_client(*(client_fd->to_client));
					std::cerr << "client error!" << std::endl;
					close(i);
				}
				else if (fd->getType() == RESOURCE_FDTYPE)
				{
					//std::cout << "resource error!" << std::endl;
					Client *client = dynamic_cast<ResourceFD*>(fd)->to_client;

					client->getResponse().makeErrorResponse(500, NULL);
					delete fd;
					MANAGER->getFDTable().erase(i);
					clrFDonTable(i, FD_RDWR);
					close(i);
				}
				else if (fd->getType() == PIPE_FDTYPE)
				{
					//std::cout << "cgi pipe error!" << std::endl;

					Client *client = dynamic_cast<ResourceFD*>(fd)->to_client;

					client->getResponse().makeErrorResponse(500, NULL);
					delete fd;
					MANAGER->getFDTable().erase(i);
					clrFDonTable(i, FD_WRONLY);
					close(i);
				}
			}
		}
	}
	return (true);
}

int Webserver::prepareResponse(Client &client)
{
	Location &location = this->getPerfectLocation( *client.getServer(), client.getRequest().getUri() );

	//std::cout << "location:" << location.getLocationName() << std::endl;
	// auth 체크 - 401
	if (location.getAuthKey() != ""
		&& (client.getRequest().getHeaders().count("Authorization") == 0 // decode 필요
		|| !client.getServer()->isCorrectAuth(location, client)))
	{
		//std::cout << "auth failed" << std::endl;
		client.getResponse().makeErrorResponse(401, &location);
		return (401);
	}

	// Allowed Method인지 체크 - 405
	if (std::find(location.getAllowMethods().begin(), location.getAllowMethods().end(), client.getRequest().getMethod()) == location.getAllowMethods().end())
	{
		//std::cout << "method not allowed" << std::endl;
		client.getResponse().makeErrorResponse(405, &location);
		return (405);
	}
	
	// client_max_body_size 체크
	//std::cout << "body size:[" << client.getRequest().getRawBody().length() << "]" << std::endl;
	//std::cout << "max body size:[" << location.getRequestMaxBodySize() << "]" << std::endl;
	if (client.getRequest().getRawBody().length() > static_cast<size_t>(location.getRequestMaxBodySize()))
	{
		//std::cout << "over max body size" << std::endl;
		client.getResponse().makeErrorResponse(413, &location);
		return (413);
	}

	//리다이렉션 체크
	if (location.getRedirectReturn() != -1)
	{
		//std::cout << "redirection response" << std::endl;
		client.getResponse().makeRedirectResponse(location);
		return (REDIRECT_RESPONSE);
	}

	// response
	if (client.getServer()->isCgiRequest(location, client.getRequest()))
	{
		//std::cout << "cgi response" << std::endl;
		return (CGI_RESPONSE);
	}
	else
	{
		return (this->prepareGeneralResponse(client, location));
	}
}

int Webserver::prepareGeneralResponse(Client &client, Location &location)
{
	//std::cout << "general response" << std::endl;

	std::string uri = client.getRequest().getUri().substr(0, client.getRequest().getUri().find('?'));

	
	if (client.getRequest().getMethod() == "GET" || client.getRequest().getMethod() == "HEAD" || client.getRequest().getMethod() == "POST")
	{
		//std::cout << "GET or HEAD or POST" << std::endl;

		if (uri[uri.length() - 1] != '/')
			uri += '/';
		
		std::string path;
		if (uri == "/")
			path = location.getRoot();
		else
			path = location.getRoot() + uri.substr(location.getLocationName().length());

		//std::cout << "path:[" << path << "]" << std::endl;
		struct stat sb;
		if (stat(path.c_str(), &sb) == -1)
		{
			path.erase(--(path.end())); // '/' 떼보고 stat 해봐서
			if (stat(path.c_str(), &sb) == -1) // 없으면 404
			{
				//std::cout << "file not found - 404" << std::endl;
				client.getResponse().makeErrorResponse(404, &location);
				return (404);		
			}
		} // if 안들어가면 디렉토리

		
		if (path[path.length() - 1] == '/') // 디렉토리
		{
			//std::cout << "check autoindex" << std::endl;
			std::string res = location.checkAutoIndex(path);
			if (res == "404")
			{
				client.getResponse().makeErrorResponse(404, &location);
				return (404) ;
			}
			else if (res == "Index of") //autoindex list up
			{
				client.getResponse().makeAutoIndexResponse(path);
				return (AUTOINDEX_RESPONSE) ;
			}
			else
				path = res;
		} // 파일은 무조건 존재함

		client.getRequest().setPath(path);
		
		//std::cout << "open GET - requested file" << std::endl;
		int get_fd = open(path.c_str(), O_RDONLY);
		//std::cout << "resource fd :" << get_fd << std::endl;
		ResourceFD *file_fd = new ResourceFD(RESOURCE_FDTYPE, &client);
		MANAGER->getFDTable().insert(std::pair<int, FDType*>(get_fd, file_fd));
		setFDonTable(get_fd, FD_RDONLY);
		if (this->fd_max < get_fd)
		{
			this->fd_max = get_fd;
		}
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
		{
			path.erase(--(path.end()));
		}
		struct stat sb;
		stat(path.c_str(), &sb);

		if (path[path.length() - 1] == '/' || S_ISDIR(sb.st_mode))
		{
			//std::cout << "put to directory" << std::endl;
			//std::cout << path << std::endl;
			client.getResponse().makeErrorResponse(400, &location);
			return (400);
		}

		//std::cout << "path: " << path << std::endl;
		client.getRequest().setPath(path);
		int put_fd = client.getServer()->createFileWithDirectory(path);
		//std::cout << "resource fd :" << put_fd << std::endl;
		ResourceFD *file_fd = new ResourceFD(RESOURCE_FDTYPE, &client);
		file_fd->setData(const_cast<std::string &>(client.getRequest().getRawBody()));
		MANAGER->getFDTable().insert(std::pair<int, FDType*>(put_fd, file_fd));
		setFDonTable(put_fd, FD_WRONLY);
		if (this->fd_max < put_fd)
		{
			this->fd_max = put_fd;
		}
	}
	return (GENERAL_RESPONSE);
}