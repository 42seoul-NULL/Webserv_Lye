#ifndef LOCATION_HPP
# define LOCATION_HPP

# include <list>
# include <map>
# include <iostream>

typedef enum			e_status
{
	REQUEST_RECEIVING,
	RESPONSE_READY
}						t_status;

class Location
{
	private	:
		std::string		root;
		std::list<std::string> index;
		std::list<std::string> allow_methods;
		std::map<int, std::string> error_pages;
		int				request_max_body_size;
		std::string		upload_path;
		bool			auto_index;
		std::string		cgi_extension;
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
		void			setCgiExtension(const std::string &cgi_extension);
		void			setAuthKey(const std::string &auth_key);
		void			setRedirectReturn(int redirect_return);
		void			setRedirectAddr(const std::string &redirect_addr);

		const std::string &getRoot();
		std::list<std::string> &getIndex();
		std::list<std::string> &getAllowMethods();
		int getRequestMaxBodySize();
		const std::string &getUploadPath();
		bool	getAutoIndex();
		const std::string &getCgiExtension();
		const std::string &getAuthKey();
		int		getRedirectReturn();
		const std::string &getRedirectAddr();
		std::map<int, std::string> &getErrorPages();

		//for test//
		void	show();
};

#endif