#include "../libft_cpp/libft.hpp"
#include "Manager.hpp"
#include "Type.hpp"

int	main(int ac, char **av)
{
	ac = 0;

	if(!MANAGER->parseConfig(av[1]))
		return (1);

	struct timeval timeout;

	timeout.tv_sec = 5; // last request time out 5000ms
	timeout.tv_usec = 0;

	try
	{
		MANAGER->getWebserver().initServers(200);
		MANAGER->getWebserver().run(timeout);
	}
	catch(const char *e)
	{
		std::cerr << e << '\n';
	}
}