#include "Manager.hpp"
#include "Webserver.hpp"
#include "utils.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Location.hpp"
#include "CGI.hpp"
#include <dirent.h>
#include <set>
#include <csignal>

#define GENERAL_RESPONSE 0
#define CGI_RESPONSE 1
#define REDIRECT_RESPONSE 2
#define AUTOINDEX_RESPONSE 3
#define LOG_RESPONSE 4


Webserver::Webserver() {}

Webserver::~Webserver() {}

void	Webserver::disconnect_client(Client &client)
{
	int client_socket_fd = client.getSocketFd();

	Client *client_pointer = &client;

	std::set<int> to_delete_fds;

	FDType *fd_type = NULL;
	ResourceFD *resource_fdtype = NULL;
	PipeFD *pipe_fdtype = NULL;

	for (std::map<int, FDType*>::iterator iter = MANAGER->getFDTable().begin(); iter != MANAGER->getFDTable().end(); ++iter)
	{
		fd_type = static_cast<FDType*>(iter->second);
		if ((resource_fdtype = dynamic_cast<ResourceFD*>(fd_type)))
		{
			if (resource_fdtype->getClient() == client_pointer)
			{
				to_delete_fds.insert(iter->first);
				if (resource_fdtype->getType() == CGI_RESOURCE_FDTYPE)
					kill(resource_fdtype->getPid(), SIGKILL);
			}
		}
		else if ((pipe_fdtype = dynamic_cast<PipeFD*>(fd_type)))
		{
			if (pipe_fdtype->getClient() == client_pointer)
				to_delete_fds.insert(iter->first);
		}
	}

	for (std::set<int>::const_iterator iter = to_delete_fds.begin(); iter != to_delete_fds.end(); ++iter)
		clrFDonTable(*iter, FD_RDWR);
	
	Server *server = client.getServer();
	server->getClients().erase(client_socket_fd);
	clrFDonTable(client_socket_fd, FD_RDWR);
}

bool	Webserver::initServers(int queue_size)
{
	if ((this->kq = kqueue()) == -1)
	{
		std::cerr << "kqueue() error" << std::endl;
		throw strerror(errno);
	}

	struct sockaddr_in  server_addr;
	int server_socket;
	for (std::map<int, Server>::iterator iter = MANAGER->getServerConfigs().begin(); iter != MANAGER->getServerConfigs().end(); iter++)
	{
		if ((server_socket = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		{
			std::cerr << "socket() error" << std::endl;
			throw strerror(errno);
		}
		iter->second.setSocketFd(server_socket);
		int option = 1;
		setsockopt(iter->second.getSocketFd(), SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));

		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = inet_addr(iter->second.getIP().c_str());
		server_addr.sin_port = htons(iter->second.getPort());

		if (bind(iter->second.getSocketFd(), (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
		{
			std::cerr << "bind() error" << std::endl;
			throw strerror(errno);
		}
		if (listen(iter->second.getSocketFd(), queue_size) == -1)
		{
			std::cerr << "listen() error" << std::endl;
			throw strerror(errno);
		}

		std::cout << "Server " << iter->second.getServerName() << "(" << iter->second.getIP() << ":" << iter->second.getPort() << ") started" << std::endl;
		this->servers[iter->second.getSocketFd()] = iter->second;

		// FDTable에 insert, 서버소켓은 read 와 error 만 검사.
		FDType *new_socket_fdtype = new ServerFD(SERVER_FDTYPE);
		setFDonTable(iter->second.getSocketFd(), FD_RDONLY, new_socket_fdtype);

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

bool	Webserver::run(struct timespec timeout)
{
	int new_events;
	struct kevent *curr_event;
	FDType *fd_type;

	pthread_create(&this->wait_thread, NULL, &waitThread, &MANAGER->getWaitQueue());
	pthread_detach(this->wait_thread);

	while (1)
	{
		new_events = kevent(this->kq, &MANAGER->getEventList()[0], MANAGER->getEventList().size(), this->return_events, 1024, &timeout);
		if (new_events < 0)
		{
			std::cerr << "kevent error" << std::endl;
			throw strerror(errno);
		}
		
		MANAGER->getEventList().clear();
		for (int i = 0; i < new_events; ++i)
		{
			curr_event = &this->return_events[i];

			if (MANAGER->getFDTable().count(curr_event->ident) == 0)
				continue ;
			fd_type = MANAGER->getFDTable()[curr_event->ident];

			if (curr_event->flags == EV_ERROR)
			{
				if (fd_type->getType() == SERVER_FDTYPE) // 서버 에러 - 프로그램 터짐
					throw "Server Error!";
				else if (fd_type->getType() == CLIENT_FDTYPE) // 클라이언트 에러 - 연결 해제
				{
					ClientFD *client_fd = dynamic_cast<ClientFD *>(fd_type);
					disconnect_client(*(client_fd->getClient()));
					std::cerr << "client error!" << std::endl;
				}
				else if (fd_type->getType() == RESOURCE_FDTYPE)
				{
					std::cerr << "resource error!" << std::endl;
					Client *client = dynamic_cast<ResourceFD*>(fd_type)->getClient();

					client->getResponse().makeErrorResponse(500, NULL);
					clrFDonTable(curr_event->ident, FD_RDWR);
				}
				else if (fd_type->getType() == PIPE_FDTYPE)
				{
					Client *client = dynamic_cast<ResourceFD*>(fd_type)->getClient();
					std::cerr << "pipe error!" << std::endl;

					client->getResponse().makeErrorResponse(500, NULL);
					clrFDonTable(curr_event->ident, FD_RDWR);
				}
			}
			if (curr_event->filter == EVFILT_READ)
			{
				if (fd_type->getType() == SERVER_FDTYPE)
					this->servers[curr_event->ident].acceptClient(curr_event->ident);
				else if (fd_type->getType() == CLIENT_FDTYPE)
				{
					Client *client = dynamic_cast<ClientFD*>(fd_type)->getClient();

					if (client->readRequest() == DISCONNECT_CLIENT)
					{
						this->disconnect_client(*client);
						std::cout << "disconnected: " << curr_event->ident << std::endl;
						continue ;
					}
					if (client->getStatus() == REQUEST_COMPLETE)
						this->prepareResponse(*client);
				}
				else if (fd_type->getType() == RESOURCE_FDTYPE || fd_type->getType() == CGI_RESOURCE_FDTYPE)
				{
					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd_type);
					resource_fd->getClient()->getResponse().tryMakeResponse(resource_fd, curr_event->ident, resource_fd->getClient()->getRequest(), curr_event->data);
				}
				else if (fd_type->getType() == ERROR_RESOURCE_FDTYPE)
				{
					char buf[BUFFER_SIZE + 1];
					int read_size;
					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd_type);

					read_size = read(curr_event->ident, buf, BUFFER_SIZE);
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
						clrFDonTable(curr_event->ident, FD_RDONLY);
					}
				}
			}
			else if (curr_event->filter == EVFILT_WRITE)
			{
				if (fd_type->getType() == CLIENT_FDTYPE)
				{
					// 클라이언트 Response write
					// 다른 곳에서 응답 raw_data 다 준비해놓고 여기서는 write 및 clear()만
					Client *client = dynamic_cast<ClientFD *>(fd_type)->getClient();

					if (client->getStatus() == RESPONSE_COMPLETE)
					{
						size_t res_idx = client->getResponse().getResIdx();
						int write_size = write(curr_event->ident, client->getResponse().getRawResponse().c_str() + res_idx, client->getResponse().getRawResponse().length() - res_idx);
						if (write_size == -1)
						{
							this->disconnect_client(*client);
							std::cout << "disconnected: " << curr_event->ident << std::endl;
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
				else if (fd_type->getType() == RESOURCE_FDTYPE)
				{
					// resource write - PUT
					ResourceFD *resource_fd = dynamic_cast<ResourceFD *>(fd_type);

					size_t write_idx = resource_fd->getWriteIdx();
					int write_size = write(curr_event->ident, resource_fd->getData().c_str() + write_idx, resource_fd->getData().length() - write_idx);
					if (write_size == -1)
					{
						std::cerr << "temporary resource write error!" << std::endl;
						continue ;
					}
					resource_fd->setWriteIdx(write_idx + write_size);
					if (resource_fd->getWriteIdx() >= resource_fd->getData().length())
					{
						resource_fd->getClient()->getResponse().makePutResponse(resource_fd->getClient()->getRequest());
						clrFDonTable(curr_event->ident, FD_WRONLY);
					}
				}
				else if (fd_type->getType() == PIPE_FDTYPE)
				{
					// cgi pipe에 body write
					PipeFD *pipefd = dynamic_cast<PipeFD *>(fd_type);

					// broken pipe 처리
					if (waitpid(pipefd->getPid(), NULL, WNOHANG) != 0)
						clrFDonTable(curr_event->ident, FD_WRONLY);
					else
					{
						int write_idx = pipefd->getWriteIdx();

						int write_size = write(curr_event->ident, pipefd->getData().c_str() + write_idx, pipefd->getData().length() - write_idx);
						if (write_size == -1)
						{
							std::cerr << "temporary pipe write error!" << std::endl;
							continue ;
						}
						pipefd->setWriteIdx(write_idx + write_size);
						if (pipefd->getWriteIdx() >= pipefd->getData().length())
							clrFDonTable(curr_event->ident, FD_WRONLY);
					}
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
	if (!location.getAllowMethods().empty() && 
		std::find(location.getAllowMethods().begin(), location.getAllowMethods().end(), client.getRequest().getMethod()) == location.getAllowMethods().end())
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
		#ifdef BONUS
			if (uri.find("/cookie_test") != std::string::npos)
			{
				client.getResponse().makeLogResponse();
				return (LOG_RESPONSE);
			}
		#endif

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
				client.getResponse().makeAutoIndexResponse(path, client.getRequest().getUri(), location);
				return (AUTOINDEX_RESPONSE) ;
			}
			else
				path = res;
		}

		client.getRequest().setPath(path);

		int get_fd = open(path.c_str(), O_RDONLY);
		if (get_fd == -1)
		{
			client.getResponse().makeErrorResponse(500, &location);
			return (500);
		}

		ResourceFD *file_fd = new ResourceFD(RESOURCE_FDTYPE, &client);
		setFDonTable(get_fd, FD_RDONLY, file_fd);
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
		if (put_fd == -1)
		{
			client.getResponse().makeErrorResponse(500, &location);
			return (500);
		}
		ResourceFD *file_fd = new ResourceFD(RESOURCE_FDTYPE, &client, client.getRequest().getRawBody());
		setFDonTable(put_fd, FD_WRONLY, file_fd);
	}
	else if (client.getRequest().getMethod() == "DELETE")
	{
		if (uri[uri.length() - 1] != '/')
			uri += '/';
		if (uri == location.getLocationName())
		{
			client.getRequest().setPath(location.getRoot());
			client.getServer()->cleanUpLocationRoot(client, location.getRoot(), location);
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
				client.getResponse().makeErrorResponse(500, &location);
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
	else
		client.getResponse().makeErrorResponse(501, &location);
	return (GENERAL_RESPONSE);
}

void deleteServerResoureces(int signo)
{
	std::set<int> fds;

	for (std::map<int, FDType*>::const_iterator iter = MANAGER->getFDTable().begin(); iter != MANAGER->getFDTable().end(); ++iter)
		fds.insert(iter->first);

	for (std::set<int>::const_iterator iter = fds.begin(); iter != fds.end(); ++iter)
		clrFDonTable(*iter, FD_RDWR);

	if (signo == SIGINT)
		std::cout << "\n[webserv: Interrupt]" << std::endl;
	if (signo == SIGKILL)
		std::cout << "\n[webserv: Killed]" << std::endl;
	exit(signo);
}

void *waitThread(void *wait_queue)
{
	std::queue<pid_t> *queue = static_cast<std::queue<pid_t>*>(wait_queue);
	pid_t pid;
	while (1)
	{
		usleep(1000);
		if (!queue->empty())
		{
			pthread_mutex_lock(&MANAGER->getWaitQueueMutex());
			pid = queue->front();
			queue->pop();
			pthread_mutex_unlock(&MANAGER->getWaitQueueMutex());
			waitpid(pid, NULL, 0);
		}
	}
	return (NULL);
}