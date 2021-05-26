#include <iostream>
#include <sys/stat.h>

int main()
{
	struct stat sb;
	int ret = stat("/Users/hyeonski/Webserv_Lye/webserv/", &sb);
	std::cout << ret << std::endl;
}