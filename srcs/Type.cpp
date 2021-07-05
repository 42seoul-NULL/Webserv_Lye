#include "Client.hpp"
#include "Manager.hpp"
#include "Type.hpp"

int FDType::getType()
{
	return (this->type);
}

ServerFD::ServerFD(t_FDType type)
{
	this->type = type;
}

ClientFD::ClientFD(t_FDType type, Client *client)
{
	this->type = type;
	this->client = client;
}

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




void setFDonTable(int fd, t_fdset set)
{
	if (set == FD_RDONLY)
		FT_FD_SET(fd, &(MANAGER->getReads()));
	else if (set == FD_WRONLY)
		FT_FD_SET(fd, &(MANAGER->getWrites()));
	else if (set == FD_RDWR)
	{
		FT_FD_SET(fd, &(MANAGER->getReads()));
		FT_FD_SET(fd, &(MANAGER->getWrites()));
	}
	FT_FD_SET(fd, &(MANAGER->getErrors()));
}

void clrFDonTable(int fd, t_fdset set)
{
	(void)set;
	FT_FD_CLR(fd, &(MANAGER->getReads()));
	FT_FD_CLR(fd, &(MANAGER->getWrites()));
	FT_FD_CLR(fd, &(MANAGER->getErrors()));
}
