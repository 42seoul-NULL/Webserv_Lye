#include "CGI.hpp"
#include "Manager.hpp"
#include "Request.hpp"
#include "Location.hpp"
#include "Client.hpp"

CGI::CGI(void)
{
	this->pid = 0;
	pipe(this->request_fd);
	this->CGI_environment_list.clear();
}

CGI::~CGI(void)
{

}

void	CGI::testCGICall(Request& request, Location& location, std::string& file_name)
{
	this->setCGIEnvironmentList(request);
	char **env = this->setCGIEnvironment(request, location);
	this->response_file_fd = open((".res_" + ft_itoa(request.getClient()->getSocketFd())).c_str(), O_CREAT | O_TRUNC | O_RDWR, 0777);

	this->pid = fork();
	// write(this->request_fd[1], request.getRawBody().c_str(), request.getRawBody().length()); // block check
	if (this->pid == 0)
	{
		// write(fd1[1], request.getRawBody().c_str(), request.getRawBody().length()); // read/write block check
		// Manager::getInstance()->getFDTable()[this->request_fd] = new ResourceFD(PIPE_FDTYPE);
		close(this->request_fd[1]);
		dup2(this->request_fd[0], 0);
		dup2(this->response_file_fd, 1);
		if (file_name.substr(file_name.find('.')) == ".php")
		{
			char **lst = (char **)malloc(sizeof(char *) * 3);
			lst[0] = strdup("./php-mac/bin/php-cgi");
			lst[1] = strdup(file_name.c_str());
			lst[2] = NULL;
			if (execve("./php-mac/bin/php-cgi", lst, env) == -1) // select()에서 write될 때까지 기다리겠지?
			{
				std::cerr << "PHP CGI EXECUTE ERROR" << std::endl;
				exit(1);
			}
		}
		else 
		{
			char **lst = (char **)malloc(sizeof(char *) * 2);
			lst[0] = strdup("./cgi-bin/cgi_tester");
			lst[1] = NULL;
			if (execve("./cgi-bin/cgi_tester", lst, env) == -1)
			{
				std::cerr << "CGI EXECUTE ERROR" << std::endl;
				exit(1);
			}
		}
		exit(0);
	}
	else
	{
		// close(this->request_fd[0]);
		//pipe fd fd_table에 insert
		PipeFD *pipe_fd = new PipeFD(PIPE_FDTYPE, this->pid, request.getClient());
		pipe_fd->setData( const_cast<std::string &>(request.getRawBody()) );

		pipe_fd->fd_read = request_fd[0];

		Manager::getInstance()->getFDTable().insert(std::pair<int, FDType *>(this->request_fd[1], pipe_fd));
		FT_FD_SET(this->request_fd[1], &(Manager::getInstance()->getWrites())); // pipe는 writes만 해주면 될 듯
		FT_FD_SET(this->request_fd[1], &(Manager::getInstance()->getErrors()));

		if (Manager::getInstance()->getWebserver().getFDMax() < this->request_fd[1])
		{
			Manager::getInstance()->getWebserver().setFDMax(this->request_fd[1]);
		}
		
		//reponse_file_fd fd_table에 insert
		ResourceFD *resource_fd = new ResourceFD(CGI_RESOURCE_FDTYPE, this->pid, request.getClient());
		Manager::getInstance()->getFDTable().insert(std::pair<int, FDType *>(this->response_file_fd, resource_fd));
		std::cout << this->response_file_fd << std::endl;
		FT_FD_SET(this->response_file_fd, &(Manager::getInstance()->getReads())); // reponse file은 reads 해주면 될 듯
		FT_FD_SET(this->response_file_fd, &(Manager::getInstance()->getErrors()));
		if (Manager::getInstance()->getWebserver().getFDMax() < this->response_file_fd)
		{
			Manager::getInstance()->getWebserver().setFDMax(this->response_file_fd);
		}
		return ;
	}
}

int		*CGI::getRequestFD(void) const
{
	return (const_cast<int *>(this->request_fd));// this->request_fd
}

int		CGI::getResponseFileFD(void) const
{
	return (this->response_file_fd);
}

void	CGI::setCGIEnvironmentList(Request& request)
{
	this->CGI_environment_list.push_back("AUTH_TYPE");
	this->CGI_environment_list.push_back("CONTENT_LENGTH");
	this->CGI_environment_list.push_back("CONTENT_TYPE");
	this->CGI_environment_list.push_back("GATEWAY_INTERFACE");
	this->CGI_environment_list.push_back("PATH_INFO");
	this->CGI_environment_list.push_back("PATH_TRANSLATED");
	this->CGI_environment_list.push_back("QUERY_STRING");
	this->CGI_environment_list.push_back("REMOTE_ADDR");
	this->CGI_environment_list.push_back("REMOTE_IDENT");
	this->CGI_environment_list.push_back("REMOTE_USER");
	this->CGI_environment_list.push_back("REQUEST_METHOD");
	this->CGI_environment_list.push_back("REQUEST_URI");
	this->CGI_environment_list.push_back("SCRIPT_NAME");
	this->CGI_environment_list.push_back("SERVER_NAME");
	this->CGI_environment_list.push_back("SERVER_PORT");
	this->CGI_environment_list.push_back("SERVER_PROTOCOL");
	this->CGI_environment_list.push_back("SERVER_SOFTWARE");

	for (std::map<std::string, std::string>::const_iterator iter = request.getHeaders().begin(); iter != request.getHeaders().end(); iter++)
	{
		this->CGI_environment_list.push_back("HTTP_" + iter->first);
	}
}

char	**CGI::setCGIEnvironment(Request& request, Location &location)
{
	std::map<std::string, std::string> cgi_env;

	if (request.getHeaders()["Authorization"] != "")
	{
		std::size_t found = request.getHeaders()["Authorization"].find(' ');
		cgi_env.insert(std::pair<std::string, std::string>("AUTH_TYPE", request.getHeaders()["Authorization"].substr(0, found)));
		// cgi_env.insert(std::pair<std::string, std::string>("REMOTE_USER", )) // auth 의 id
	}
	std::cout << request.getHeaders()["Content-Length"] << std::endl;
	if (request.getHeaders()["Content-Length"] != "")
		cgi_env.insert(std::pair<std::string, std::string>("CONTENT_LENGTH", request.getHeaders()["Content-Length"]));
	else if (request.getHeaders()["Transfer-Encoding"] == "chunked")
		cgi_env.insert(std::pair<std::string, std::string>("CONTENT_LENGTH", ft_itoa(request.getRawBody().length())));
	else
		cgi_env.insert(std::pair<std::string, std::string>("CONTENT_LENGTH", "0"));	

	if (request.getHeaders()["Content-Type"] != "")
		cgi_env.insert(std::pair<std::string, std::string>("CONTENT_TYPE", request.getHeaders()["Content-Length"]));
	cgi_env.insert(std::pair<std::string, std::string>("GATEWAY_INTERFACE", "Cgi/1.1"));

	//////////// PARSE_PATH_INFO
	std::size_t	front_pos = request.getUri().find('.');
	std::size_t back_pos = request.getUri().find('?');
	std::string path_info = request.getUri().substr(front_pos, back_pos - front_pos);

	if ((front_pos = path_info.find('/')) != std::string::npos)
		path_info = path_info.substr(front_pos);
	else
		path_info = request.getUri();

	cgi_env.insert(std::pair<std::string, std::string>("PATH_INFO", path_info)); // 잠시
	///////////// PARSE_PATH_INFO

	cgi_env.insert(std::pair<std::string, std::string>("PATH_TRANSLATED", location.getRoot() + path_info.substr(1)));

	if (request.getUri().find('?') != std::string::npos)
		cgi_env.insert(std::pair<std::string, std::string>("QUERY_STRING", request.getUri().substr(request.getUri().find('?'))));
	
	// cgi_env.insert(std::pair<std::string, std::string>("REMOTE_ADDR", )) // client IP Address -> should ^^
	// cgi_env.insert(std::pair<std::string, std::string>("REMOTE_IDENT", )) // 조사 필요 NSCA -> should ^^
	cgi_env.insert(std::pair<std::string, std::string>("REQUEST_METHOD", request.getMethod()));
	cgi_env.insert(std::pair<std::string, std::string>("REQUEST_URI", request.getUri()));

	///////// SCRIPT_NAME
	if (request.getUri() == path_info)
		cgi_env.insert(std::pair<std::string, std::string>("SCRIPT_NAME", location.getRoot() + path_info.substr(1)));
	else if (request.getUri().find(path_info) == std::string::npos)
	{
		cgi_env.insert(std::pair<std::string, std::string>("SCRIPT_NAME", location.getRoot() + request.getUri().substr(1))); // 임시
	}
	else
	{
		std::size_t pos = request.getUri().rfind(path_info);
		cgi_env.insert(std::pair<std::string, std::string>("SCRIPT_NAME", location.getRoot() + request.getUri().substr(1, pos - 1)));
	}
	std::cout << cgi_env.find("SCRIPT_NAME")->second << std::endl;
	///////// SCRIPT_NAME
	
	cgi_env.insert(std::pair<std::string, std::string>("SERVER_NAME", "127.0.0.1"));
	// cgi_env.insert(std::pair<std::string, std::string>("SERVER_PORT", "8180")); // 상의
	cgi_env.insert(std::pair<std::string, std::string>("SERVER_PROTOCOL", "HTTP/1.1"));
	cgi_env.insert(std::pair<std::string, std::string>("SERVER_SOFTWARE", "HyeonSkkiDashi/1.0")); 
	// http header insert
	for (std::map<std::string, std::string>::const_iterator iter = request.getHeaders().begin(); iter != request.getHeaders().end(); iter++)
	{
		cgi_env.insert(std::pair<std::string, std::string>("HTTP_" + iter->first, iter->second));
	}

	return (makeCGIEnvironment(cgi_env));
}

char	**CGI::makeCGIEnvironment(std::map<std::string, std::string> &cgi_env)
{
	char	**env;
	int		i = 0;

	env = (char **)malloc(sizeof(char *) * (cgi_env.size() + 1));
	for (std::map<std::string, std::string>::iterator iter = cgi_env.begin(); iter != cgi_env.end(); iter++)
	{
		std::string tmp = std::string(iter->first + "=" + iter->second);
		env[i] = strdup(tmp.c_str());
		i++;
	}
	env[i] = NULL;
	return (env);
}