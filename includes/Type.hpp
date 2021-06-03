#ifndef TYPE_HPP
# define TYPE_HPP

# include <string>

# define BUFFER_SIZE 65536
# define MANAGER Manager::getInstance()

class Client;

typedef enum			e_status
{
	REQUEST_RECEIVING,
	REQUEST_COMPLETE,
	RESPONSE_COMPLETE,
}						t_status;

typedef enum	e_FDType
{
	SERVER_FDTYPE,
	CLIENT_FDTYPE,
	RESOURCE_FDTYPE,
	PIPE_FDTYPE,
	CGI_RESOURCE_FDTYPE,
	ERROR_RESOURCE_FDTYPE,
}				t_FDType;

typedef enum	e_fdset
{
	FD_RDONLY,
	FD_WRONLY,
	FD_RDWR,
}				t_fdset;

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
	private:
		Client *client;

	public:
		ClientFD(t_FDType type, Client *client);
		~ClientFD() {}
		
		Client *getClient(void);
};

class ResourceFD : public FDType
{
	private:
		const std::string *data;
		size_t write_idx;
		pid_t pid;
		Client *client;

	public :
		ResourceFD(t_FDType type, pid_t pid, Client *client);
		ResourceFD(t_FDType type, Client *client);
		ResourceFD(t_FDType type, Client *client, const std::string &data);
		~ResourceFD() {}

		Client *getClient(void);
		pid_t getPid(void);
		const std::string &getData(void);
		size_t getWriteIdx(void);

		void setWriteIdx(size_t write_idx);
};

class PipeFD : public FDType
{
	private:
		const std::string &data;
		int write_idx;
		pid_t pid;
		Client *client;
		int fd_read;

	public:
		PipeFD(t_FDType type, pid_t pid, Client *client, const std::string &data);
		~PipeFD() {}
		
		Client *getClient(void);
		pid_t getPid(void);
		int getFdRead(void);
		const std::string &getData();
		int getWriteIdx();

		void setWriteIdx(int write_idx);
		void setFdRead(int fd);
};

void setFDonTable(int fd, t_fdset set);
void clrFDonTable(int fd, t_fdset set);

#endif
