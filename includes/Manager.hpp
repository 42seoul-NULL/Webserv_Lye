#ifndef MANAGER_HPP
# define MANAGER_HPP

# include <map>
# include <iostream>
# include "utils.hpp"
# include "Webserver.hpp"
# include "Server.hpp"
# include "Type.hpp"

class Client;
class FDType;

class Manager
{
	private:
		Manager();
		Manager(const Manager &src);
		Manager& operator=(const Manager& src);
		bool returnFalseWithMsg(const char *str);
		bool isReserved(const std::string &src);

		std::map<int, Server> server_configs;
		static Manager*	instance;
		std::map<std::string, std::string> mime_type;
		std::map<std::string, std::string> status_code;

		std::multimap<int, struct kevent> event_map;
		std::map<int, FDType*> fd_table;

		Webserver webserver;
		void initMimeType(void);
		void initStatusCode(void);

	public:
		virtual ~Manager();
		static Manager* getInstance();
		static const int decodeMimeBase64[256];
		
		std::map<int, Server> &getServerConfigs();
		std::map<std::string, std::string> &getMimeType();	
		std::map<std::string, std::string> &getStatusCode();
		Webserver &getWebserver();
		std::multimap<int, struct kevent> &getEventMap();
		std::map<int, FDType*> &getFDTable();

		bool parseConfig(const char *config_file_path);
		int decode_base64(const char *text, char *dst, int numBytes);

};

#endif