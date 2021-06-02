#include "../libft_cpp/libft.hpp"
#include "Manager.hpp"
#include "Type.hpp"

int	main(int argc, char **argv)
{
	(void)argc;

	if (MANAGER->parseConfig(argv[1]) == 0)
		return (EXIT_FAILURE);
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	try
	{
		MANAGER->getWebserver().initServers(256);
		MANAGER->getWebserver().run(timeout);
	}
	catch(const char *e)
	{
		std::cerr << e << '\n';
	}
	return (EXIT_SUCCESS);
}