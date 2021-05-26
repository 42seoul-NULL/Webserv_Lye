#include "Manager.hpp"
#include "Webserver.hpp"
#include "../libft_cpp/libft.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Location.hpp"
#include "CGI.hpp"


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

	FT_FD_CLR(client_socket_fd, &(Manager::getInstance()->getReads()));
	FT_FD_CLR(client_socket_fd, &(Manager::getInstance()->getWrites()));
	FT_FD_CLR(client_socket_fd, &(Manager::getInstance()->getErrors()));
	close(client_socket_fd);
	
	Server *server = client.getServer();
	Client *client_pointer = &client;
	server->getClients().erase(client_socket_fd);


	ResourceFD *resource_fdtype = NULL;
	PipeFD *pipe_fdtype = NULL;
	for (std::map<int, FDType*>::iterator iter = Manager::getInstance()->getFDTable().begin(); iter != Manager::getInstance()->getFDTable().end(); )
	{
		if ((resource_fdtype = dynamic_cast<ResourceFD *>(iter->second)))
		{
			if (resource_fdtype->to_client == client_pointer)
			{
				FT_FD_CLR(iter->first, &(Manager::getInstance()->getReads()));
				FT_FD_CLR(iter->first, &(Manager::getInstance()->getWrites()));
				FT_FD_CLR(iter->first, &(Manager::getInstance()->getErrors()));
				delete resource_fdtype;
				Manager::getInstance()->getFDTable().erase(iter->first);
			}
			else
				iter++;
		}
		else if ((pipe_fdtype = dynamic_cast<PipeFD *>(iter->second)))
		{
			if (pipe_fdtype->to_client == client_pointer)
			{
				FT_FD_CLR(iter->first, &(Manager::getInstance()->getReads()));
				FT_FD_CLR(iter->first, &(Manager::getInstance()->getWrites()));
				FT_FD_CLR(iter->first, &(Manager::getInstance()->getErrors()));
				delete pipe_fdtype;
				Manager::getInstance()->getFDTable().erase(iter->first);
			}
			else
				iter++;
		}
		else
			iter++;
	}

	delete Manager::getInstance()->getFDTable()[client_socket_fd];
	Manager::getInstance()->getFDTable()[client_socket_fd] = NULL;
	Manager::getInstance()->getFDTable().erase(client_socket_fd);

	return ;
}

bool	Webserver::initServers(int queue_size)
{
	FT_FD_ZERO(&(Manager::getInstance()->getReads()));
	FT_FD_ZERO(&(Manager::getInstance()->getWrites()));
	FT_FD_ZERO(&(Manager::getInstance()->getErrors()));

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
		if (Manager::getInstance()->getWebserver().getFDMax() < iter->second.getSocketFd())
		{
			Manager::getInstance()->getWebserver().setFDMax(iter->second.getSocketFd());
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

	std::cout << uri << std::endl;

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
	std::cout << uri_loc << std::endl;
	// std::cout << ret.getLocationName() << std::endl;
	for (std::map<std::string, Location>::iterator iter = loc_map.begin(); iter != loc_map.end(); iter++)
	{
		std::cout << iter->second.getLocationName() << std::endl;
		std::cout << iter->second.getRoot() << std::endl;
	}
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
		// std::cout << "check select" << std::endl;

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
						std::cout << "disconnected: " << i << std::endl;
						continue ;
					}
					if (client->getStatus() == REQUEST_COMPLETE)
					{
						Location &location = this->getPerfectLocation( *client->getServer(), client->getRequest().getUri() );

						std::cout << "location:" << location.getLocationName() << std::endl;
						std::cout << "check auth" << std::endl;
						// auth 체크 - 401
						if (location.getAuthKey() != ""
							&& (client->getRequest().getHeaders().count("Authorization") == 0 // decode 필요
							|| !client->getServer()->isCorrectAuth(location, *client)))
						{
							client->getResponse().makeErrorResponse(401, &location);
							continue ;
						}
				
						std::cout << "check method" << std::endl;
						// Allowed Method인지 체크 - 405
						if (std::find(location.getAllowMethods().begin(), location.getAllowMethods().end(), client->getRequest().getMethod()) == location.getAllowMethods().end())
						{
							client->getResponse().makeErrorResponse(405, &location);
							continue ;
						}
						
						std::cout << "check max body" << std::endl;
						// client_max_body_size 체크
						if (client->getRequest().getRawBody().length() > static_cast<size_t>(location.getRequestMaxBodySize()))
						{
							client->getResponse().makeErrorResponse(413, &location);
							continue ;
						}

						std::cout << "check cgi" << std::endl;
						// response
						if (client->getServer()->isCgiRequest(location, client->getRequest()))
							continue ;
						else
						{
							std::cout << "general response" << std::endl;
							// 일반 response 처리 필요
							// 일반 response는 어디까지 여기서 처리해줘야 할까?
							std::string uri = client->getRequest().getUri().substr(0, client->getRequest().getUri().find('?'));
							// Location &location = this->getPerfectLocation(*client->getServer(), uri);

							std::cout << "check redirection" << std::endl;
							//리다이렉션 체크
							if (location.getRedirectReturn() != -1)
							{
								client->getResponse().makeRedirectResponse(location);
								continue ;
							}


							// if (uri[uri.length() - 1] != '/')
							// 	uri += '/';
							
							// struct stat sb;
							// std::string path = location.getRoot() + uri.substr(location.getLocationName().length());

							// if (stat(path.c_str(), &sb) == -1)
							// {
							// 	path.erase(--(path.end())); // '/' 떼보고 stat 해봐서
							// 	if (stat(path.c_str(), &sb) == -1) // 없으면 404
							// 	{
							// 		client->getResponse().makeErrorResponse(404, &location);
							// 		continue ;		
							// 	}
							// } // if 안들어가면 디렉토리

							

							if (client->getRequest().getMethod() == "GET" || client->getRequest().getMethod() == "HEAD")
							{
								std::cout << "get or head" << std::endl;


								if (uri[uri.length() - 1] != '/')
									uri += '/';
								
								struct stat sb;
								std::string path;
								if (uri == "/")
									path = location.getRoot();
								else
									path = location.getRoot() + uri.substr(location.getLocationName().length());

								if (stat(path.c_str(), &sb) == -1)
								{
									path.erase(--(path.end())); // '/' 떼보고 stat 해봐서
									if (stat(path.c_str(), &sb) == -1) // 없으면 404
									{
										client->getResponse().makeErrorResponse(404, &location);
										continue ;		
									}
								} // if 안들어가면 디렉토리

								std::cout << "check autoindex" << std::endl;
								
								if (path[path.length() - 1] == '/') // 디렉토리
								{
									std::string res = location.checkAutoIndex(path);
									if (res == "404")
									{
										client->getResponse().makeErrorResponse(404, &location);
										continue ;
									}
									else if (res == "Index of") //autoindex list up
									{
										client->getResponse().makeAutoIndexResponse(path);
										continue ;
									}
									else
										path = res;
								} // 파일은 무조건 존재함

								client->getRequest().setPath(path);
								
								std::cout << "open get - requested file" << std::endl;
								int get_fd = open(path.c_str(), O_RDONLY);
								std::cout << "resource fd :" << get_fd << std::endl;
								ResourceFD *file_fd = new ResourceFD(RESOURCE_FDTYPE, client);
								Manager::getInstance()->getFDTable().insert(std::pair<int, FDType*>(get_fd, file_fd));
								FT_FD_SET(get_fd, &(Manager::getInstance()->getReads()));
								FT_FD_SET(get_fd, &(Manager::getInstance()->getErrors()));
								if (this->fd_max < get_fd)
								{
									this->fd_max = get_fd;
								}
								std::cout << "client read cycle end" << std::endl;
							}
							else if (client->getRequest().getMethod() == "PUT" || client->getRequest().getMethod() == "POST")
							{
								// std::string uri = client->getRequest().getUri().substr(0, client->getRequest().getUri().find('?'));
								// Location &location = this->getPerfectLocation(*client->getServer(), uri);

								// if (location.getRedirectReturn() != -1) 
								// {
								// 	client->getResponse().makeRedirectResponse(location);
								// 	continue ;
								// }
								// std::string filename = location.getLocationName();
								// if (uri.find(filename) == std::string::npos)
								// {
								// 	client->getResponse().makeErrorResponse(404, &location); // asdf
								// 	continue ;
								// }
								// filename.erase(0, filename.length());
								
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

								

								if (path[path.length() - 1] == '/')
								{
									std::cout << "put to directory" << std::endl;
									std::cout << path << std::endl;
									client->getResponse().makeErrorResponse(400, &location);
									continue ;
								}

								client->getRequest().setPath(path);
								int put_fd = client->getServer()->createFileWithDirectory(path);
								std::cout << "resource fd :" << put_fd << std::endl;
								ResourceFD *file_fd = new ResourceFD(RESOURCE_FDTYPE, client);
								file_fd->setData(const_cast<std::string &>(client->getRequest().getRawBody()));
								Manager::getInstance()->getFDTable().insert(std::pair<int, FDType*>(put_fd, file_fd));
								FT_FD_SET(put_fd, &(Manager::getInstance()->getWrites()));
								FT_FD_SET(put_fd, &(Manager::getInstance()->getErrors()));
								if (this->fd_max < put_fd)
								{
									this->fd_max = put_fd;
								}
							}
						}
					}
				}
				else if (fd->getType() == RESOURCE_FDTYPE || fd->getType() == CGI_RESOURCE_FDTYPE)
				{
					// resource read
					// pid 확인 후 exit이면 read 후 response 작성
					// exit이 아니라면 continue;
					std::cout << "try to read resource" << std::endl;
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
				else if (fd->getType() == ERROR_RESOURCE_FDTYPE)
				{
					std::cout << "try to read error resource" << std::endl;

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
						Manager::getInstance()->getFDTable().erase(i);
						FT_FD_CLR(i, &(Manager::getInstance()->getReads()));
						FT_FD_CLR(i, &(Manager::getInstance()->getErrors()));
					}
				}
			}
			else if (FT_FD_ISSET(i, &cpy_writes))
			{
				if (fd->getType() == CLIENT_FDTYPE)
				{
					// std::cout << "select: client write" << std::endl;
					//if  (ft_get_time() - ) // 타임아웃 체크 필요 // 그냥 하지 말자...
					Client *client = dynamic_cast<ClientFD *>(fd)->to_client;
					// 클라이언트 Response write
					// 다른 곳에서 응답 raw_data 다 준비해놓고 여기서는 write 및 clear()만
					// std::cout << "try to response" << std::endl;
					if (client->getStatus() == RESPONSE_COMPLETE)
					{
						std::cout << "ready to response" << std::endl;
						if (client->getResponse().getRawResponse().length() > BUFFER_SIZE)
						{
							int write_size = write(i, client->getResponse().getRawResponse().c_str(), BUFFER_SIZE);
							client->getResponse().getRawResponse().erase(0, write_size);
							continue ;
						}
						else
						{
							int write_size = write(i, client->getResponse().getRawResponse().c_str(), client->getResponse().getRawResponse().length());
							if (static_cast<size_t>(write_size) < client->getResponse().getRawResponse().length())
							{
								client->getResponse().getRawResponse().erase(0, write_size);
								continue ;
							}
							// std::cout << client->getResponse().getRawResponse() << std::endl;
							client->getResponse().getRawResponse().clear();
							client->getRequest().initRequest();
							client->getResponse().initResponse();
							client->setStatus(REQUEST_RECEIVING);
						}
						std::cout << "finished response" << std::endl;
					}
				}
				else if (fd->getType() == RESOURCE_FDTYPE)
				{
					std::cout << "try to write resource" << std::endl;
					// resource write

					// 일반 응답에 대한 resource일 경우 (PUT 또는 POST겠지)
					// 정해진 데이터 write만
					// fd_table delete 해야함
					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd);
					if (resource_fd->getData().length() > BUFFER_SIZE)
					{
						int write_size = write(i, resource_fd->getData().c_str(), BUFFER_SIZE);
						resource_fd->getData().erase(0, write_size);
						continue ;
					}
					else
					{
						// std::cout << "data: " << resource_fd->getData() << std::endl;
						int write_size = write(i, resource_fd->getData().c_str(), resource_fd->getData().length());
						if (static_cast<size_t>(write_size) < resource_fd->getData().length())
						{
							resource_fd->getData().erase(0, write_size);
							continue ;
						}

						resource_fd->getData().clear();
						resource_fd->to_client->getResponse().tryMakePutResponse(resource_fd->to_client->getRequest());
						delete fd;
						Manager::getInstance()->getFDTable()[i] = NULL;
						Manager::getInstance()->getFDTable().erase(i);
						FT_FD_CLR(i, &(Manager::getInstance()->getWrites()));
						FT_FD_CLR(i, &(Manager::getInstance()->getErrors()));
						close(i);
					}
				}
				else if (fd->getType() == PIPE_FDTYPE)
				{
					std::cout << "try to write cgi pipe" << std::endl;

					// cgi pipe에 body write
					PipeFD *pipefd = dynamic_cast<PipeFD *>(fd);

					if (pipefd->getData().length() > BUFFER_SIZE)
					{
						int write_size = write(i, pipefd->getData().c_str(), BUFFER_SIZE);
						pipefd->getData().erase(0, write_size);
						continue ;
					}
					else
					{
						int write_size = write(i, pipefd->getData().c_str(), pipefd->getData().length());
						if (static_cast<size_t>(write_size) < pipefd->getData().length())
						{
							pipefd->getData().erase(0, write_size);
							continue ;
						}
						pipefd->getData().clear();

						close(pipefd->fd_read);
						close(i);
						delete fd;
						Manager::getInstance()->getFDTable()[i] = NULL;
						Manager::getInstance()->getFDTable().erase(i);
						FT_FD_CLR(i, &(Manager::getInstance()->getWrites()));
						FT_FD_CLR(i, &(Manager::getInstance()->getErrors()));

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
					disconnect_client(*(client_fd->to_client));
					std::cerr << "client error!" << std::endl;
					close(i);
				}
				else if (fd->getType() == RESOURCE_FDTYPE)
				{
					std::cout << "resource error!" << std::endl;
					Client *client = dynamic_cast<ResourceFD*>(fd)->to_client;

					client->getResponse().makeErrorResponse(500, NULL);
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
					std::cout << "cgi pipe error!" << std::endl;

					Client *client = dynamic_cast<ResourceFD*>(fd)->to_client;

					client->getResponse().makeErrorResponse(500, NULL);
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
