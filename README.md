# Webserv

스끼다시 먹으러 갑시다

### HTTP/1.1 Webserver

This is a HTTP server implemented with C++, using kqueue model.

- how to use

  - clone this repository
  - `make all`
  - `./webserv [config_file_path]`

- how to set server config file

  - example

  - ```
    server
    {
    	server_name [server_name]
    	listen [port] [address]
    	
    	location /
    	{
    		error_page [status_code] [error_page_path]
    		allow_methods [METHOD] [...]
    		root [path]
    		index [file1] [file2] [...]
    		auto_index [on/off]
    		
    		cgi_info [.extension] [executable_path]
    		cgi_info .php /usr/bin/php-cgi
    	}
    	
    	location [location_name]
    	{
    		request_max_body_size [size]
    		return [status_code] [redirect_address]
    		auth_key [ID]:[PW]
    	}
    }
    ```

  - config file needs at least 1 `server {}`

  - each server {} block must has `server_name`, `listen`

  - `server_name`  must be followed by `listen` 

  - each server needs at least 1 `location {}`

  - **All reserved words** except server_name, listen must be located in `location {}`

- how to test
  - functional test : `./cgi-bin/tester http://[address]:[port]`
  - stress test(needs siege): `siege -R <(echo connection = keep-alive) -c[client_size] http://[address]:[port]`

- etc)
  - supports session management
    - logging urls that the client had requested
    - `make fclean && make bonus`
    - `./webserv configs/test.conf`
    - request GET /cookie_test
