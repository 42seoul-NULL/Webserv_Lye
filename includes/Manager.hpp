#ifndef MANAGER_HPP
# define MANAGER_HPP

# include <map>
# include <iostream>
# include "Server.hpp"
# include "../includes/Webserver.hpp"
# include "../libft_cpp/libft.hpp"

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
		virtual ~FDType(void);
		int getType();
};

class ServerFD : public FDType
{
	public :
		ServerFD(int type);
};

class ClientFD : public FDType
{
	public :
		ClientFD(int type);
		Client *to_client;
};

class ResourceFD : public FDType
{
	public :
		ResourceFD(int type);
};

class Manager
{
	private :
		Manager();
		Manager(const Manager &src);
		Manager& operator=(const Manager& src);
		bool	returnFalseWithMsg(const char *str);
		bool	isReserved(const std::string &src);

		std::map<int, Server> servers; // config용
		static Manager*	instance;
		std::map<std::string, std::string> mime_type;
		std::map<std::string, std::string> status_code;
		std::map<int, FDType *> fd_table;
		Webserver webserver;

	public	:
		virtual ~Manager();
		static Manager* getInstance();
		static const int decodeMimeBase64[256];
		
		std::map<int, Server>& getServers();
		std::map<std::string, std::string>& getMimeType();	
		std::map<std::string, std::string>& getStatusCode();
		bool	parseConfig(const char *config_file_path);
		std::map<int, FDType *> &getFDTable();
		Webserver &getWebserver();
		//for test
		void	show();
};

#endif