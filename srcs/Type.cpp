#include "Client.hpp"
#include "Manager.hpp"
#include "Type.hpp"
#include <sys/types.h>
#include <sys/event.h>

int FDType::getType()
{
	return (this->type);
}

FDType::~FDType() {}


ServerFD::ServerFD(t_FDType type)
{
	this->type = type;
}

ServerFD::~ServerFD() {}


ClientFD::ClientFD(t_FDType type, Client *client)
{
	this->type = type;
	this->client = client;
}

ClientFD::~ClientFD() {}

Client *ClientFD::getClient(void)
{
	return (this->client);
}


ResourceFD::ResourceFD(t_FDType type, Client *client)
{
	this->type = type;
	this->client = client;
	this->data = NULL;
	this->write_idx = 0;
}

ResourceFD::ResourceFD(t_FDType type, Client *client, const std::string &data)
{
	this->type = type;
	this->client = client;
	this->write_idx = 0;
	this->data = &data;
}

ResourceFD::ResourceFD(t_FDType type, pid_t pid, Client *client)
{
	this->type = type;
	this->pid = pid;
	this->client = client;
	this->data = NULL;
	this->write_idx = 0;
}

ResourceFD::~ResourceFD() {}

Client *ResourceFD::getClient(void)
{
	return (this->client);
}

pid_t ResourceFD::getPid(void)
{
	return (this->pid);
}

const std::string &ResourceFD::getData()
{
	return (*this->data);
}

size_t ResourceFD::getWriteIdx()
{
	return (this->write_idx);
}

void ResourceFD::setWriteIdx(size_t write_idx)
{
	this->write_idx = write_idx;
}

PipeFD::PipeFD(t_FDType type, pid_t pid, Client *client, const std::string &data) : data(data)
{
	this->type = type;
	this->pid = pid;
	this->client = client;
	this->write_idx = 0;
}

PipeFD::~PipeFD() {}

Client *PipeFD::getClient(void)
{
	return (this->client);
}

pid_t PipeFD::getPid(void)
{
	return (this->pid);
}

const std::string &PipeFD::getData()
{
	return (this->data);
}

int PipeFD::getWriteIdx()
{
	return (this->write_idx);
}

void PipeFD::setWriteIdx(int write_idx)
{
	this->write_idx = write_idx;
}




void setFDonTable(int fd, t_fdset set, void *read_data, void *write_data)
{
	struct kevent event;

	if (set == FD_RDONLY)
	{
		EV_SET(&event, fd, EVFILT_READ, EV_ADD | EV_ENABLE, NULL, NULL, read_data);
		MANAGER->getEventMap().insert(std::pair<int, struct kevent>(fd, event));
	}
	else if (set == FD_WRONLY)
	{
		EV_SET(&event, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, NULL, NULL, write_data);
		MANAGER->getEventMap().insert(std::pair<int, struct kevent>(fd, event));
	}
	else if (set == FD_RDWR)
	{
		EV_SET(&event, fd, EVFILT_READ, EV_ADD | EV_ENABLE, NULL, NULL, read_data);
		MANAGER->getEventMap().insert(std::pair<int, struct kevent>(fd, event));
		EV_SET(&event, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, NULL, NULL, write_data);
		MANAGER->getEventMap().insert(std::pair<int, struct kevent>(fd, event));

	}
}

void clrFDonTable(int fd, t_fdset set)
{
	(void)set;

	std::multimap<int, struct kevent>::iterator lower = MANAGER->getEventMap().lower_bound(fd);
	std::multimap<int, struct kevent>::iterator upper = MANAGER->getEventMap().upper_bound(fd);

	while (lower != upper)
	{
		delete static_cast<FDType*>(lower->second.udata);
		lower->second.udata = NULL;
		++lower;
	}
	MANAGER->getEventMap().erase(fd);
	close(fd);
}
