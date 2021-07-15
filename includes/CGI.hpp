#ifndef CGI_HPP
# define CGI_HPP

# include <iostream>
# include <vector>
# include <map>

class Location;
class Request;

class CGI
{
	private:
		int	request_fd[2];
		int response_fd[2];
		pid_t	pid;

	public:
		CGI(void);
		virtual ~CGI(void);

		int		*getRequestFD(void) const;
		int		*getResponseFD(void) const;

		void	testCGICall(Request& request, Location& location, std::string& file_name, const std::string &exec_name);
		char	**setCGIEnvironment(Request& request, Location &location, std::string &file_path);
		char	**makeCGIEnvironment(std::map<std::string, std::string> &cgi_env);
};

#endif