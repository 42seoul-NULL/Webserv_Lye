#ifndef UTILS_HPP
# define UTILS_HPP

# include <stdlib.h>
# include <fcntl.h>
# include <unistd.h>
# include <stddef.h>
# include <sys/time.h>
# include <string>
# include <vector>


bool				ft_split(const std::string &target, const std::string& sep, std::vector<std::string> &saver);
std::string			ft_itoa(int n);
int					ft_atoi_hex(const std::string &str);
int 				ft_remove_directory(std::string dir);
int					ft_nbr_length(int n);

#endif
