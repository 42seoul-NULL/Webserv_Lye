#ifndef MANAGER_HPP
# define MANAGER_HPP

# include <map>
# include <iostream>
# include "../libft_cpp/libft.hpp"
# include "Webserver.hpp"
# include "Server.hpp"

class Client;

typedef enum	e_FDType
{
	SERVER_FDTYPE,
	CLIENT_FDTYPE,
	RESOURCE_FDTYPE,
}				t_FDType;

class FDType
{
	protected:
		int type;
	public :
		virtual ~FDType() {}
		int getType();
};

class ServerFD : public FDType
{
	public :
		ServerFD(int type);
		~ServerFD() {}
};

class ClientFD : public FDType
{
	public :
		ClientFD(int type, Client *client);
		Client *to_client;
		~ClientFD() {}
};

class ResourceFD : public FDType
{
	public :
		ResourceFD(int type);
		~ResourceFD() {}
};

class Manager
{
	private :
		Manager();
		Manager(const Manager &src);
		Manager& operator=(const Manager& src);
		bool	returnFalseWithMsg(const char *str);
		bool	isReserved(const std::string &src);

		std::map<int, Server> server_configs; // config용
		static Manager*	instance;
		std::map<std::string, std::string> mime_type;
		std::map<std::string, std::string> status_code;
		std::map<int, FDType *> fd_table;
		Webserver webserver;

	public	:
		virtual ~Manager();
		static Manager* getInstance();
		static const int decodeMimeBase64[256];
		
		std::map<int, Server>& getServerConfigs();
		std::map<std::string, std::string>& getMimeType();	
		std::map<std::string, std::string>& getStatusCode();
		bool	parseConfig(const char *config_file_path);
		std::map<int, FDType *> &getFDTable();
		Webserver &getWebserver();
		//for test
		void	show();
};

#endif