#include "CGI.hpp"
#include "Manager.hpp"
#include "Request.hpp"
#include "Location.hpp"

CGI::CGI(void)
{
	pipe(this->request_fd);
	this->response_file_fd = open(("res" + ft_itoa(this->request_fd[1])).c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
	this->setCGIEnvironmentList();//어케할까여?
}

CGI::~CGI(void)
{

}

void	CGI::testCGICall(const Request& request, Location& location, std::string& file_name)
{
	char **env = this->setCGIEnvironment(request, location);

	this->pid = fork();
	//pipe fd fd_table에 insert
	PipeFD *pipe_fd = new PipeFD(PIPE_FDTYPE, this->pid, request.getClient());
	pipe_fd->setData( const_cast<std::string &>(request.getRawBody()) );
	Manager::getInstance()->getFDTable().insert(std::pair<int, FDType *>(this->request_fd[1], pipe_fd));

	ResourceFD *resource_fd = new ResourceFD(CGI_RESOURCE_FDTYPE, this->pid, request.getClient());
	Manager::getInstance()->getFDTable().insert(std::pair<int, FDType *>(this->response_file_fd, resource_fd));
	// write(this->request_fd[1], request.getRawBody().c_str(), request.getRawBody().length()); // block check
	if (this->pid == 0)
	{
		// write(fd1[1], request.getRawBody().c_str(), request.getRawBody().length()); // read/write block check
		// Manager::getInstance()->getFDTable()[this->request_fd] = new ResourceFD(PIPE_FDTYPE);
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
		Manager::getInstance()->getFDTable().insert(std::pair<int, FDType *>(this->response_file_fd, new ResourceFD(CGI_RESOURCE_FDTYPE, this->pid, request.getClient())));
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

void	CGI::setCGIEnvironmentList(const Request& request)
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

char	**CGI::setCGIEnvironment(const Request& request, Location &location)
{
	std::map<std::string, std::string> cgi_env;

	if (request.getHeaders()["Authorization"] != "")
	{
		std::size_t found = request.getHeaders()["Authorization"].find(' ');
		cgi_env.insert(std::pair<std::string, std::string>("AUTH_TYPE", request.getHeaders()["Authorization"].substr(0, found)));
		// cgi_env.insert(std::pair<std::string, std::string>("REMOTE_USER", )) // auth 의 id
	}
	if (request.getHeaders()["Content-Length"] != "")
		cgi_env.insert(std::pair<std::string, std::string>("CONTENT_LENGTH", request.getHeaders()["Content-Length"]));
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
	cgi_env.insert(std::pair<std::string, std::string>("SERVER_SOFTWARE", "GOLDPIG/1.0")); 
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