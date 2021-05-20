#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <iostream>
# include <unistd.h>
# include "Manager.hpp"
# include "Request.hpp"
# include "Response.hpp"

# define BUFFER_SIZE 65536

typedef enum			e_status
{
	REQUEST_RECEIVING,
	REQUEST_COMPLETE,
}						t_status;

class Client
{
	private	:
		t_status		status;
		int				server_socket_fd;
		int				socket_fd;
		long long		remain_body;
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
		void		setRemainBody(long long remain_body);
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

		void readRequest(void);

};

#endif