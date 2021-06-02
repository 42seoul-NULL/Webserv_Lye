#ifndef WEBSERVER_HPP
# define WEBSERVER_HPP

# include <iostream>
# include <string>
# include <stdlib.h>
# include <unistd.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <sys/select.h>
# include <errno.h>
# include <string>
# include <fcntl.h>
# include <sys/stat.h>
# include <map>

class Server;
class Client;
class Location;

class Webserver
{
	private :
		int		fd_max;
		std::map<int, Server> servers;

		void	disconnect_client(Client &client);
		const Server &getServerFromClient(int server_socket_fd, const std::string &server_name);

	public	:
		Webserver();
		virtual ~Webserver();
		Webserver(const Webserver &src);
		Webserver 	&operator=(const Webserver &src);

		void		setFDMax(int fd_max);
		int			getFDMax(void);

		Location	&getPerfectLocation(Server &server, const std::string &uri);
		bool		initServers(int queue_size);
		bool		run(struct timeval timeout);
		int 		prepareResponse(Client &client);
		int 		prepareGeneralResponse(Client &client, Location &location);
};

#endif