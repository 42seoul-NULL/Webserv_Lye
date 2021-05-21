#include <iostream>
#include <map>

int main()
{
	std::map<int, int> test;

	std::cout << test.size() << std::endl;
	if (test[3] == 0)
		std::cout << "asf" << std::endl;
	std::cout << test.size() << std::endl;
}