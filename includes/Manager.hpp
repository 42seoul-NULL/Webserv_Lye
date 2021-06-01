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
	PIPE_FDTYPE,
	CGI_RESOURCE_FDTYPE,
	ERROR_RESOURCE_FDTYPE,
}				t_FDType;

class FDType
{
	protected:
		t_FDType type;
	public :
		virtual ~FDType() {}
		int getType();
};

class ServerFD : public FDType
{
	public :
		ServerFD(t_FDType type);
		~ServerFD() {}
};

class ClientFD : public FDType
{
	public :
		ClientFD(t_FDType type, Client *client);
		Client *to_client;
		~ClientFD() {}
};

class ResourceFD : public FDType
{
	private:
		std::string data;
	public :
		ResourceFD(t_FDType type, pid_t pid, Client *client);
		ResourceFD(t_FDType type, Client *client);
		pid_t pid;
		Client *to_client;
		std::string &getData();
		void setData(std::string &data);
		~ResourceFD() {}
};

class PipeFD : public FDType
{
	private:
		const std::string &data;
		int write_idx;
	public:
		PipeFD(t_FDType type, pid_t pid, Client *client, const std::string &data);
		pid_t pid;
		Client *to_client;
		int fd_read;

		const std::string &getData();
		int getWriteIdx();

		void setWriteIdx(int write_idx);
		~PipeFD() {}
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
		fd_set	reads;
		fd_set	writes;
		fd_set	errors;
		Webserver webserver;
		void	initMimeType(void);
		void	initStatusCode(void);

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
		fd_set &getReads(void);
		fd_set &getWrites(void);
		fd_set &getErrors(void);
		int decode_base64(const char * text, char * dst, int numBytes);

		//for test
		void	show();
};

#endif