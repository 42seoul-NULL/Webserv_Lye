#include "libft.hpp"
#include "Manager.hpp"
#include "Type.hpp"

int	main(int argc, char **argv)
{
	(void)argc;

	if (MANAGER->parseConfig(argv[1]) == 0)
		return (EXIT_FAILURE);
	struct timespec timeout;
	timeout.tv_sec = 5;
	timeout.tv_nsec = 0;
	try
	{
		signal(SIGINT, deleteServerResoureces);
		signal(SIGKILL, deleteServerResoureces);
		MANAGER->getWebserver().initServers(256);
		MANAGER->getWebserver().run(timeout);
	}
	catch(const char *e)
	{
		std::cerr << e << '\n';
	}
	return (EXIT_SUCCESS);
}