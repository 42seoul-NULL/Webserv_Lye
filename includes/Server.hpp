#ifndef SERVER_HPP
# define SERVER_HPP

# include <iostream>
# include <map>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <sys/select.h>
# include <fcntl.h>
# include "../libft_cpp/libft.hpp"

class Request;
class Location;
class Client;

class Server
{
	private	:
		std::string		ip;
		unsigned short	port;
		std::string		server_name;
		int				socket_fd;
		std::map<std::string, Location> locations;
		std::map<int, Client> clients;

	public	:
		Server();
		Server(const Server &src);
		Server& operator=(const Server &src);
		virtual	~Server();

		void	setPort(unsigned short port);
		void	setIP(const std::string &ip);
		void	setServerName(const std::string &server_name);
		void	setSocketFd(int socket_fd);

		const std::string &getIP() const;
		const std::string &getServerName() const;
		unsigned short	   getPort() const;
		int				   getSocketFd() const;
		std::map<int, Client> &getClients();

		std::map<std::string, Location> &getLocations();
		int acceptClient(int server_fd, int &fd_max);
		bool isCgiRequest(Location &location, Request &request);
		int	createFileWithDirectory(std::string path);
		bool isCorrectAuth(Location &location, Client &client);

		//for test//
		void	show();
};

#endif