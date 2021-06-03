#ifndef MANAGER_HPP
# define MANAGER_HPP

# include <map>
# include <iostream>
# include "libft.hpp"
# include "Webserver.hpp"
# include "Server.hpp"
# include "Type.hpp"

class Client;
class FDType;

class Manager
{
	private :
		Manager();
		Manager(const Manager &src);
		Manager& operator=(const Manager& src);
		bool	returnFalseWithMsg(const char *str);
		bool	isReserved(const std::string &src);

		std::map<int, Server> server_configs;
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
		void	deleteFromFDTable(int fd, FDType *fd_type, t_fdset set);

		bool	parseConfig(const char *config_file_path);
		std::map<int, FDType *> &getFDTable();
		Webserver &getWebserver();
		fd_set &getReads(void);
		fd_set &getWrites(void);
		fd_set &getErrors(void);
		int decode_base64(const char * text, char * dst, int numBytes);

};

#endif