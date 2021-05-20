#include "../includes/Location.hpp"

Location::Location() : request_max_body_size(-1), redirect_return(-1)
{
	
}

Location::Location(const Location &src)
{
	this->root	=	src.root;
	this->index.assign(src.index.begin(), src.index.end());
	this->allow_methods.assign(src.allow_methods.begin(), src.allow_methods.end());
	this->request_max_body_size = src.request_max_body_size;
	this->error_pages.insert(src.error_pages.begin(), src.error_pages.end());
	this->upload_path = src.upload_path;
	this->auto_index = src.auto_index;
	this->cgi_extension = src.cgi_extension;
	this->auth_key = src.auth_key;
	this->redirect_addr = src.redirect_addr;
	this->redirect_return = src.redirect_return;
}

Location &Location::operator=(const Location &src)
{
	this->root	=	src.root;
	this->index.assign(src.index.begin(), src.index.end());
	this->allow_methods.assign(src.allow_methods.begin(), src.allow_methods.end());
	this->request_max_body_size = src.request_max_body_size;
	this->error_pages.insert(src.error_pages.begin(), src.error_pages.end());
	this->upload_path = src.upload_path;
	this->auto_index = src.auto_index;
	this->cgi_extension = src.cgi_extension;
	this->auth_key = src.auth_key;
	this->redirect_addr = src.redirect_addr;
	this->redirect_return = src.redirect_return;
	return (*this);
}

void		Location::setRoot(const std::string& root)
{
	this->root = root;
	return ;
}

void		Location::setRequestMaxBodySize(int request_max_body_size)
{
	this->request_max_body_size = request_max_body_size;
	return ;
}
void		Location::setUploadPath(const std::string &upload_path)
{
	this->upload_path = upload_path;
	return ;
}

void		Location::setAutoIndex(bool auto_index)
{
	this->auto_index = auto_index;
	return ;
}

void		Location::setCgiExtension(const std::string &cgi_extension)
{
	this->cgi_extension = cgi_extension;
	return ;
}

void		Location::setAuthKey(const std::string &auth_key)
{
	this->auth_key = auth_key;
	return ;
}

void		Location::setRedirectReturn(int redirect_return)
{
	this->redirect_return = redirect_return;
	return ;
}

void		Location::setRedirectAddr(const std::string &redirect_addr)
{
	this->redirect_addr = redirect_addr;
	return ;
}

const std::string &Location::getRoot()
{
	return (this->root);
}

std::list<std::string> &Location::getIndex()
{
	return (this->index);
}

std::list<std::string> &Location::getAllowMethods()
{
	return (this->allow_methods);
}

int Location::getRequestMaxBodySize()
{
	return (this->request_max_body_size);
}

std::map<int, std::string> &Location::getErrorPages()
{
	return (this->error_pages);
}

const std::string &Location::getUploadPath()
{
	return (this->upload_path);
}

bool	Location::getAutoIndex()
{
	return (this->auto_index);
}

const std::string &Location::getCgiExtension()
{
	return (this->cgi_extension);
}

const std::string &Location::getAuthKey()
{
	return (this->auth_key);
}

int		Location::getRedirectReturn()
{
	return (this->redirect_return);
}

const std::string &Location::getRedirectAddr()
{
	return (this->redirect_addr);
}

//for test
void	Location::show()
{
	std::cout << "root	:	" << this->root << std::endl;
	std::cout << "rqmbs	:	" << this->request_max_body_size << std::endl;
	std::cout << "upload_path	:	" << this->upload_path << std::endl;
	std::cout << "auto_index	:	" << this->auto_index << std::endl;
	std::cout << "cgi_extension	:	" << this->cgi_extension << std::endl;
	std::cout << "auth_key	:	" << this->auth_key << std::endl;
	std::cout << "index	: ";
	for (std::list<std::string>::iterator iter = this->index.begin(); iter != this->index.end(); iter++)
		std::cout << *iter << " ";
	std::cout << std::endl;
	std::cout << "allow_methods	: ";
	for (std::list<std::string>::iterator iter = this->allow_methods.begin(); iter != this->allow_methods.end(); iter++)
		std::cout << *iter << " ";
	std::cout << std::endl;
	std::cout << "error_pages	: " << std::endl;
	for (std::map<int, std::string>::iterator iter = this->error_pages.begin(); iter != this->error_pages.end(); iter++)
		std::cout << iter->first << " | " << iter->second << std::endl;
}