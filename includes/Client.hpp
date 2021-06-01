#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <iostream>
# include <unistd.h>
# include "Type.hpp"
# include "Request.hpp"
# include "Response.hpp"

# define DISCONNECT_CLIENT -1

class Server;

class Client
{
	private	:
		t_status		status;
		int				server_socket_fd;
		int				socket_fd;
		unsigned long	last_request_ms;
		Request			request;
		Response		response;
		Server			*server;

	public	:
		Client();
		Client(int server_socket_fd, int socket_fd);
		~Client();
		// Client(const Location &src);
		// Client& operator=(const Location &src);

		void		setSocketFd(int socket_fd);
		void		setServerSocketFd(int server_socket_fd);
		void		setStatus(t_status status);
		void		setLastRequestMs(unsigned long last_request_ms);
		void		setServer(Server &server);

		int			getSocketFd();
		int			getServerSocketFd();
		t_status	getStatus();
		long long	getRemainBody();
		unsigned long getLastRequestMs();
		Request		&getRequest();
		Response	&getResponse();
		Server		*getServer();

		int readRequest(void);

};

#endif