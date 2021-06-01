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
		int	response_file_fd;
		pid_t	pid;

	public:
		CGI(void);
		virtual ~CGI(void);

		int		*getRequestFD(void) const;
		int		getResponseFileFD(void) const;

		void	testCGICall(Request& request, Location& location, std::string& extension);
		char	**setCGIEnvironment(Request& request, Location &location);
		char	**makeCGIEnvironment(std::map<std::string, std::string> &cgi_env);
};

#endif