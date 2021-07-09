#include <unistd.h>
#include <dirent.h>
#include <iostream>
#include "utils.hpp"

int ft_atoi_hex(const std::string &str)
{
	int result = 0;

	for (std::string::const_iterator iter = str.begin(); iter != str.end(); iter++)
	{
		if ( *iter >= '0' && *iter <= '9' )
		{
			result = result * 16 + (*iter - '0');
		}
		else if ( *iter >= 'a' && *iter <= 'f')
		{
			result = result * 16 + (*iter - 'a' + 10);
		}
		else if ( *iter >= 'A' && *iter <= 'F')
		{
			result = result * 16 + (*iter - 'A' + 10);
		}
		else
			break ;
	}
	return (result);
}

std::string ft_itoa(int n)
{
	int		len;
	int		i;
	int		temp;
	unsigned char	buf[12];

	len = ft_nbr_length(n);
	if (n == 0)
		buf[0] = '0';
	else
	{
		i = len - 1;
		while (n != 0)
		{
			temp = n % 10;
			buf[i--] = temp < 0 ? (-temp + '0') : (temp + '0');
			n /= 10;
		}
		if (i == 0)
			buf[0] = '-';
	}
	buf[len] = 0;
	return (std::string((const char *)buf));
}

bool				ft_split(const std::string &target, const std::string& sep, std::vector<std::string> &saver)
{
	std::string temp;

	for (std::string::const_iterator iter = target.begin(); iter != target.end(); iter++)
	{
		if (sep.find(*iter) == std::string::npos )
			temp += *iter;
		else
		{
			if (temp != "")
			{
				saver.push_back(temp);
				temp.clear();
			}
		}
	}
	if (temp != "")
		saver.push_back(temp);
	return (true);
}


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

int		ft_nbr_length(int n)
{
	int i;

	i = 0;
	if (n <= 0)
		i++;
	while (n != 0)
	{
		n /= 10;
		i++;
	}
	return (i);
}
