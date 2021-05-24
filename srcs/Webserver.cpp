#include "Manager.hpp"
#include "Webserver.hpp"
#include "../libft_cpp/libft.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Location.hpp"
#include "CGI.hpp"

Webserver::Webserver() : fd_max(-1)
{
	FT_FD_ZERO(&(Manager::getInstance()->getReads()));
	FT_FD_ZERO(&(Manager::getInstance()->getWrites()));
	FT_FD_ZERO(&(Manager::getInstance()->getErrors()));
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

void	Webserver::disconnect_client(Client &client)
{
	int client_socket_fd = client.getSocketFd();

	FT_FD_CLR(client_socket_fd, &(Manager::getInstance()->getReads()));
	FT_FD_CLR(client_socket_fd, &(Manager::getInstance()->getWrites()));
	FT_FD_CLR(client_socket_fd, &(Manager::getInstance()->getErrors()));
	close(client_socket_fd);
	
	Server *server = client.getServer();
	server->getClients().erase(client_socket_fd);

	delete Manager::getInstance()->getFDTable()[client_socket_fd];
	Manager::getInstance()->getFDTable()[client_socket_fd] = NULL;
	Manager::getInstance()->getFDTable().erase(client_socket_fd);

	return ;
}

// const Server &Webserver::getServerFromClient(int server_socket_fd, const std::string &server_name)
// {
// 	if (this->servers[server_socket_fd].find(server_name) == this->servers[server_socket_fd].end())
// 		return (this->servers[server_socket_fd].begin()->second);
// 	return ( (this->servers[server_socket_fd][server_name]) );
// }

bool	Webserver::initServers(int queue_size)
{
	for (std::map<int, Server>::iterator iter = Manager::getInstance()->getServerConfigs().begin(); iter != Manager::getInstance()->getServerConfigs().end(); iter++)
	{
		struct sockaddr_in  server_addr;

		iter->second.setSocketFd(socket(PF_INET, SOCK_STREAM, 0));

		// FDTable에 insert
		FDType *new_socket_fdtype = new ServerFD(SERVER_FDTYPE);
		Manager::getInstance()->getFDTable().insert(std::pair<int, FDType*>(iter->second.getSocketFd(), new_socket_fdtype));
		
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
		FT_FD_SET(iter->second.getSocketFd(), &(Manager::getInstance()->getReads()));
		FT_FD_SET(iter->second.getSocketFd(), &(Manager::getInstance()->getErrors()));

		
		this->servers[iter->second.getSocketFd()] = iter->second;

		if (this->fd_max < iter->second.getSocketFd())
			this->fd_max = iter->second.getSocketFd();
	}
	return (true);
}

// Location &Webserver::getPerfectLocation(int server_socket_fd, const std::string &server_name, const std::string &uri)
// {
// 	std::map<std::string, Location> *locs;

// 	if (this->servers[server_socket_fd].find(server_name) == this->servers[server_socket_fd].end()) // 못찾으면
// 		locs = &(this->servers[server_socket_fd].begin()->second.getLocations());
// 	else
// 		locs = &(this->servers[server_socket_fd][server_name].getLocations());

// 	Location &ret = (*locs)["/"];

// 	std::string key = "";
// 	for (std::string::const_iterator iter = uri.begin(); iter != uri.end(); iter++)
// 	{
// 		key += *iter;
// 		if (*iter == '/')
// 		{
// 			if (locs->find(key) == locs->end())
// 				return (ret);
// 			else
// 				ret = (*locs)[key];
// 		}
// 	}
// 	return (ret);
// }

Location &Webserver::getPerfectLocation(Server &server, const std::string &uri)
{
	size_t pos;
	std::string uri_loc = "";

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
	Location &ret = loc_map["/"];

	std::string key = "";
	for (std::string::const_iterator iter = uri_loc.begin(); iter != uri_loc.end(); iter++)
	{
		key += *iter;
		if (*iter == '/')
		{
			if (loc_map.find(key) == loc_map.end())
			{
				return (ret);
			}
			else
				ret = loc_map[key];
		}
	}
	return (ret);	
}

bool	Webserver::run(struct timeval timeout, unsigned int buffer_size)
{
	fd_set	cpy_reads;
	fd_set	cpy_writes;
	fd_set	cpy_errors;

	int fd_num;

	while (1)
	{
		usleep(5); // cpu 점유가 100% 까지 올라가는 것을 막기 위해서

		cpy_reads = Manager::getInstance()->getReads();
		cpy_writes = Manager::getInstance()->getWrites();
		cpy_errors = Manager::getInstance()->getErrors();

		if ( (fd_num = select(this->fd_max + 1, &cpy_reads, &cpy_writes, &cpy_errors, &timeout)) == -1)
			throw strerror(errno);

		if (fd_num == 0)
			continue ;
		
		FDType *fd = NULL;
		for (int i = 0; i <= this->fd_max; i++)
		{	
			if (Manager::getInstance()->getFDTable().count(i) == 0) // fd_table에 없는 fd라면 continue
				continue ;
			fd = Manager::getInstance()->getFDTable()[i];

			if (FT_FD_ISSET(i, &cpy_reads))
			{
				if (fd->getType() == SERVER_FDTYPE)
				{
					int client_socket_fd = this->servers[i].acceptClient(i, this->fd_max);
					FT_FD_SET(client_socket_fd, &(Manager::getInstance()->getReads()));
					FT_FD_SET(client_socket_fd, &(Manager::getInstance()->getWrites()));
					FT_FD_SET(client_socket_fd, &(Manager::getInstance()->getErrors()));
				}
				else if (fd->getType() == CLIENT_FDTYPE)
				{
					Client *client = dynamic_cast<ClientFD *>(fd)->to_client;

					//if  (ft_get_time() - ) // 타임아웃 체크 필요 // 그냥 하지 말자...

					if (client->readRequest() == DISCONNECT_CLIENT)
					{
						disconnect_client(*client);
						std::cout << "disconnected: " << i << std::endl;
						continue ;
					}
					if (client->getStatus() == REQUEST_COMPLETE)
					{
						Location &location = this->getPerfectLocation( *client->getServer(), client->getRequest().getUri() );
						// response
						if (client->getServer()->isCgiRequest(location,  client->getRequest()))
						{
							// cgi 처리 필요
							CGI cgi;
							cgi.testCGICall(client->getRequest(), location, /*extension 필요*/ );
							
						}
						else
						{
							
							// 일반 response 처리 필요
							// 일반 response는 어디까지 여기서 처리해줘야 할까?

							// auth 체크 - 401
							// Allowed Method인지 체크 - 405
							
							// client_max_body_size 체크
							std::string uri = client->getRequest().getUri().substr(0, client->getRequest().getUri().find('?'));
							Location &location = this->getPerfectLocation(*client->getServer(), uri);
							if (client->getRequest().getRawBody().length() == static_cast<size_t>(location.getRequestMaxBodySize()))
							{
								client->getResponse().makeErrorResponse(413);
								continue ;
							}

							if (client->getRequest().getMethod() == "GET" || client->getRequest().getMethod() == "HEAD")
							{
								std::string uri = client->getRequest().getUri().substr(0, client->getRequest().getUri().find('?'));
								Location &location = this->getPerfectLocation(*client->getServer(), uri);
								std::string path;

								if (location.getRedirectReturn() != -1)
								{
									client->getResponse().makeRedirectResponse(location);
									continue ;
								}

								std::string uri_autocheck = uri;
								if (uri_autocheck[uri_autocheck.length() - 1] != '/')
								{
									uri_autocheck += '/';
									if (uri_autocheck == location.getLocationName())
									{
										location.checkAutoIndex(uri_autocheck); // true / false => return => 처리
									}
									else // 하위 폴더
									{
										// std::size_t pos = uri_autocheck.find(location.getLocationName());
										path = location.getRoot() + uri_autocheck.substr(location.getLocationName().length());
										
										struct stat sb;
										if (stat(path.c_str(), &sb) == -1)
										{
											client->getResponse().makeErrorResponse(404);
										}
									}
								}
								else // 무조건 uri 폴더
								{
									location.checkAutoIndex(uri_autocheck);
									// res 따라서 404 / index Of / index file
								}
								
								// list page 만들기
								// 1. 클라이언트가 직접 파일 요청    2. index.html 존재해서 찾아줬음

								client->getRequest().setPath(path);
								
								int fd = open(path.c_str(), O_RDONLY);
								ResourceFD *file_fd = new ResourceFD(RESOURCE_FDTYPE, client);
								Manager::getInstance()->getFDTable().insert(std::pair<int, FDType*>(fd, file_fd));
								FT_FD_SET(fd, &(Manager::getInstance()->getReads()));
								FT_FD_SET(fd, &(Manager::getInstance()->getErrors()));
							}
							else if (client->getRequest().getMethod() == "PUT" || client->getRequest().getMethod() == "POST")
							{
								std::string uri = client->getRequest().getUri().substr(0, client->getRequest().getUri().find('?'));
								Location &location = this->getPerfectLocation(*client->getServer(), uri);

								if (location.getRedirectReturn() != -1)
								{
									client->getResponse().makeRedirectResponse(location);
									continue ;
								}
								std::string filename = location.getLocationName();
								if (uri.find(filename) == std::string::npos)
								{
									client->getResponse().makeErrorResponse(404); // asdf
									continue ;
								}
								filename.erase(0, filename.length());

								std::string path = location.getRoot() + filename;
								if (path[path.length() - 1] == '/')
								{
									client->getResponse().makeErrorResponse(400);
								}
								//file 만드는 함수 만들어주세요...
								// from jayun
							}
							
						}
					}
				}
				else if (fd->getType() == RESOURCE_FDTYPE || fd->getType() == CGI_RESOURCE_FDTYPE)
				{
					// resource read
					// pid 확인 후 exit이면 read 후 response 작성
					// exit이 아니라면 continue;
					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd);
					
					resource_fd->to_client->getResponse().tryMakeResponse(resource_fd, i, resource_fd->to_client->getRequest());

					// tryMakeResponse에서 일어나야하는 일들
					// 	CGI Resource라면
					// 		CGI 프로세스가 EXIT 상태가 아니라면
					// 			return;
					// 	read(resource);
					// 	delete resource_fd;
					// 	FD_CLR();

					// 	makeResponse() - 에러 나면 500
					// 	setStatus(RESPONSE_READY)

						

				}
			}
			else if (FT_FD_ISSET(i, &cpy_writes))
			{
				if (fd->getType() == CLIENT_FDTYPE)
				{
					//if  (ft_get_time() - ) // 타임아웃 체크 필요 // 그냥 하지 말자...
					Client *client = dynamic_cast<ClientFD *>(fd)->to_client;
					// 클라이언트 Response write
					// 다른 곳에서 응답 raw_data 다 준비해놓고 여기서는 write 및 clear()만
					if (client->getStatus() == RESPONSE_COMPLETE)
					{
						if (client->getResponse().getRawResponse().length() > BUFFER_SIZE)
						{
							write(i, client->getResponse().getRawResponse().c_str(), BUFFER_SIZE);
							client->getResponse().getRawResponse().erase(0, BUFFER_SIZE);
							continue ;
						}
						else
						{
							write(i, client->getResponse().getRawResponse().c_str(), client->getResponse().getRawResponse().length());
							client->getResponse().getRawResponse().clear();
						}
						client->getRequest().initRequest();
						client->getResponse().initResponse();
						client->setStatus(REQUEST_RECEIVING);
					}
				}
				else if (fd->getType() == RESOURCE_FDTYPE)
				{
					// resource write

					// 일반 응답에 대한 resource일 경우 (PUT 또는 POST겠지)
					// 정해진 데이터 write만
					// fd_table delete 해야함
					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd);
					if (resource_fd->getData().length() > BUFFER_SIZE)
					{
						write(i, resource_fd->getData().c_str(), BUFFER_SIZE);
						resource_fd->getData().erase(0, BUFFER_SIZE);
						continue ;
					}
					else
					{
						write(i, resource_fd->getData().c_str(), resource_fd->getData().length());
						resource_fd->getData().clear();
						delete fd;
						Manager::getInstance()->getFDTable[i] = NULL;
						Manager::getInstance()->getFDTable.erase(i);
					}
				}
				else if (fd->getType() == PIPE_FDTYPE)
				{
					// cgi pipe에 body write
					PipeFD *pipefd = dynamic_cast<PipeFD *>(fd);

					if (pipefd->getData().length() > BUFFER_SIZE)
					{
						write(i, pipefd->getData().c_str(), BUFFER_SIZE);
						pipefd->getData().erase(0, BUFFER_SIZE);
						continue ;
					}
					else
					{
						write(i, pipefd->getData().c_str(), pipefd->getData().length());
						pipefd->getData().clear();
						delete fd;
						Manager::getInstance()->getFDTable[i] = NULL;
						Manager::getInstance()->getFDTable.erase(i);
					}
					// pipe fd일 경우
					// 얘도 정해진 데이터 (request body)만 write하고 fd delete
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
					disconnect_client(*(client_fd->to_client));
					std::cerr << "client error!" << std::endl;
					close(i);
				}
				else if (fd->getType() == RESOURCE_FDTYPE)
				{
					Client *client = dynamic_cast<ResourceFD*>(fd)->to_client;

					client->getResponse().makeErrorResponse(500);
					delete fd;
					Manager::getInstance()->getFDTable().erase(i);
					if (FT_FD_ISSET(i, &(Manager::getInstance()->getReads())))
						FT_FD_CLR(i, &(Manager::getInstance()->getReads()));
					if (FT_FD_ISSET(i, &(Manager::getInstance()->getWrites())))
						FT_FD_CLR(i, &(Manager::getInstance()->getWrites()));
					FT_FD_CLR(i, &(Manager::getInstance()->getErrors()));
					close(i);
				}
				else if (fd->getType() == PIPE_FDTYPE)
				{
					Client *client = dynamic_cast<ResourceFD*>(fd)->to_client;

					client->getResponse().makeErrorResponse(500);
					delete fd;
					Manager::getInstance()->getFDTable().erase(i);
					FT_FD_CLR(i, &(Manager::getInstance()->getWrites()));
					FT_FD_CLR(i, &(Manager::getInstance()->getErrors()));
					close(i);
				}
			}
		}
	}
	return (true);
}
