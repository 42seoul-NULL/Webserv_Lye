#include "CGI.hpp"
#include "Manager.hpp"
#include "Request.hpp"
#include "Location.hpp"
#include "Client.hpp"

CGI::CGI(void)
{
	this->pid = 0;
	pipe(this->request_fd);
	pipe(this->response_fd);
}

CGI::~CGI(void)
{

}

void	CGI::testCGICall(Request& request, Location& location, std::string& file_name)
{
	this->pid = fork();
	if (this->pid == 0)
	{
		std::string file_path = request.getUri();
		file_path = file_path.substr(location.getLocationName().length());
		file_path = location.getRoot() + file_path;
		char **env = this->setCGIEnvironment(request, location, file_path);
		close(this->request_fd[1]);
		close(this->response_fd[0]);
		dup2(this->request_fd[0], 0);
		dup2(this->response_fd[1], 1);
		if (file_name.substr(file_name.find('.')) == ".php")
		{
			char **lst = (char **)malloc(sizeof(char *) * 3);
			lst[0] = strdup("./php-mac/bin/php-cgi");
			lst[1] = strdup(file_path.c_str());
			lst[2] = NULL;
			if (execve("./php-mac/bin/php-cgi", lst, env) == -1)
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
		close(this->request_fd[0]);
		close(this->response_fd[1]);
		fcntl(this->request_fd[1], F_SETFL, O_NONBLOCK);
		fcntl(this->response_fd[0], F_SETFL, O_NONBLOCK);
		//pipe fd fd_table에 insert
		PipeFD *pipe_fd = new PipeFD(PIPE_FDTYPE, this->pid, request.getClient(), request.getRawBody());

		MANAGER->getFDTable().insert(std::pair<int, FDType *>(this->request_fd[1], pipe_fd));
		setFDonTable(this->request_fd[1], FD_WRONLY);
		if (MANAGER->getWebserver().getFDMax() < this->request_fd[1])
		{
			MANAGER->getWebserver().setFDMax(this->request_fd[1]);
		}
		
		//reponse_fd fd_table에 insert
		ResourceFD *resource_fd = new ResourceFD(CGI_RESOURCE_FDTYPE, this->pid, request.getClient());
		MANAGER->getFDTable().insert(std::pair<int, FDType *>(this->response_fd[0], resource_fd));
		setFDonTable(this->response_fd[0], FD_RDONLY);
		if (MANAGER->getWebserver().getFDMax() < this->response_fd[0])
		{
			MANAGER->getWebserver().setFDMax(this->response_fd[0]);
		}
		return ;
	}
}

int		*CGI::getRequestFD(void) const
{
	return (const_cast<int *>(this->request_fd));
}

int		*CGI::getResponseFD(void) const
{
	return (const_cast<int *>(this->response_fd));
}

char	**CGI::setCGIEnvironment(Request& request, Location &location, std::string &file_path)
{
	std::map<std::string, std::string> cgi_env;

	std::multimap<std::string, std::string>::iterator iter = request.getHeaders().find("Authorization");
	if (iter != request.getHeaders().end() && iter->second != "")
	{
		std::size_t found = iter->second.find(' ');
		cgi_env.insert(std::pair<std::string, std::string>("AUTH_TYPE", iter->second.substr(0, found)));
	}

	iter = request.getHeaders().find("Content-Length");
	if (iter != request.getHeaders().end() && iter->second != "")
		cgi_env.insert(std::pair<std::string, std::string>("CONTENT_LENGTH", iter->second));
	else if (((iter = request.getHeaders().find("Transfer-Encoding")) != request.getHeaders().end()) && iter->second == "chunked")
		cgi_env.insert(std::pair<std::string, std::string>("CONTENT_LENGTH", ft_itoa(request.getRawBody().length())));
	else
		cgi_env.insert(std::pair<std::string, std::string>("CONTENT_LENGTH", "0"));	

	iter = request.getHeaders().find("Content-Type");
	if (iter != request.getHeaders().end() && iter->second != "")
		cgi_env.insert(std::pair<std::string, std::string>("CONTENT_TYPE", iter->second));
	cgi_env.insert(std::pair<std::string, std::string>("GATEWAY_INTERFACE", "Cgi/1.1"));

	std::size_t	front_pos = request.getUri().find('.');
	std::size_t back_pos = request.getUri().find('?');
	std::string path_info = request.getUri().substr(front_pos, back_pos - front_pos);

	if ((front_pos = path_info.find('/')) != std::string::npos)
		path_info = path_info.substr(front_pos);
	else
		path_info = request.getUri();

	cgi_env.insert(std::pair<std::string, std::string>("PATH_INFO", path_info)); // 잠시

	cgi_env.insert(std::pair<std::string, std::string>("PATH_TRANSLATED", location.getRoot() + path_info.substr(1)));

	if (request.getUri().find('?') != std::string::npos)
		cgi_env.insert(std::pair<std::string, std::string>("QUERY_STRING", request.getUri().substr(request.getUri().find('?'))));
	
	cgi_env.insert(std::pair<std::string, std::string>("REQUEST_METHOD", request.getMethod()));
	cgi_env.insert(std::pair<std::string, std::string>("REQUEST_URI", request.getUri()));

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
	
	size_t pos = file_path.find(".");
	size_t pos2 = file_path.find("/", pos);
	if (pos2 == std::string::npos)
		pos2 = file_path.find("?", pos);
	cgi_env.insert(std::pair<std::string, std::string>("SCRIPT_FILENAME", file_path.substr(0, pos2)));


	cgi_env.insert(std::pair<std::string, std::string>("SERVER_NAME", "127.0.0.1"));
	cgi_env.insert(std::pair<std::string, std::string>("SERVER_PORT", ft_itoa(request.getClient()->getServer()->getPort())));
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


