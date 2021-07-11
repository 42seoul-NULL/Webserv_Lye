#ifndef MANAGER_HPP
# define MANAGER_HPP

# include <map>
# include <list>
# include <queue>
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

		std::vector<struct kevent> event_list;
		std::map<int, FDType*> fd_table;
		std::queue<pid_t> wait_queue;

		Webserver webserver;
		void initMimeType(void);
		void initStatusCode(void);

		pthread_mutex_t wait_queue_mutex;

	public:
		virtual ~Manager();
		static Manager* getInstance();
		static const int decodeMimeBase64[256];
		
		std::map<int, Server> &getServerConfigs();
		std::map<std::string, std::string> &getMimeType();	
		std::map<std::string, std::string> &getStatusCode();
		Webserver &getWebserver();
		std::vector<struct kevent> &getEventList();
		std::map<int, FDType*> &getFDTable();
		std::queue<pid_t> &getWaitQueue();
		pthread_mutex_t &getWaitQueueMutex();

		bool parseConfig(const char *config_file_path);
		int decode_base64(const char *text, char *dst, int numBytes);

};

#endif