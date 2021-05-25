#ifndef LOCATION_HPP
# define LOCATION_HPP

# include <list>
# include <map>
# include <vector>
# include <iostream>

class Location
{
	private:
		std::string		location_name;
		std::string		root;
		std::list<std::string> index;
		std::list<std::string> allow_methods;
		std::map<int, std::string> error_pages;
		int				request_max_body_size;
		std::string		upload_path;
		bool			auto_index;
		std::vector<std::string> cgi_extensions;
		std::string		auth_key;

		int				redirect_return;
		std::string		redirect_addr;

	public	:
		Location();
		virtual ~Location(){};
		Location(const Location &src);
		Location& operator=(const Location &src);

		void			setRoot(const std::string &root);
		void			setRequestMaxBodySize(int request_max_body_size);
		void			setUploadPath(const std::string &upload_path);
		void			setAutoIndex(bool auto_index);
		void			setCgiExtensions(std::vector<std::string> &cgi_extensions);
		void			setAuthKey(const std::string &auth_key);
		void			setRedirectReturn(int redirect_return);
		void			setRedirectAddr(const std::string &redirect_addr);

		const std::string &getLocationName();
		const std::string &getRoot();
		std::list<std::string> &getIndex();
		std::list<std::string> &getAllowMethods();
		int getRequestMaxBodySize();
		const std::string &getUploadPath();
		bool	getAutoIndex();
		std::vector<std::string> &getCgiExtensions();
		const std::string &getAuthKey();
		int		getRedirectReturn();
		const std::string &getRedirectAddr();
		std::map<int, std::string> &getErrorPages();

		void setLocationName(std::string &locaton_name);

		std::string		checkAutoIndex(std::string &uri);

		//for test//
		void	show();
};

#endif