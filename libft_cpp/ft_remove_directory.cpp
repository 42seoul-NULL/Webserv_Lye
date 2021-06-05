#include <unistd.h>
#include <dirent.h>
#include <iostream>

int ft_remove_directory(std::string dir)
{
	DIR *dir_ptr;
	struct dirent *file;

	if ((dir_ptr = opendir(dir.c_str())) == NULL)
		return (1);
	if (dir[dir.length() - 1] != '/')
		dir += '/';
	std::string name;
	int ret;
	while ((file = readdir(dir_ptr)) != NULL)
	{
		name = std::string(file->d_name);
		if (name == "." || name == "..")
			continue ;
		if (file->d_type == DT_DIR)
		{
			ret = ft_remove_directory(dir + name);
			if (ret == 1)
				return (1);
		}
		else
			unlink((dir + name).c_str());
	}
	rmdir(dir.c_str());
	return (0);
}