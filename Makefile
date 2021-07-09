SRCNAME	= \
	main.cpp\
	Client.cpp\
	Location.cpp\
	Manager.cpp\
	Server.cpp\
	Webserver.cpp\
	Request.cpp\
	Response.cpp\
	Type.cpp\
	CGI.cpp\
	utils.cpp

SRCDIR = ./srcs/

SRCS = ${addprefix ${SRCDIR}, ${SRCNAME}}

INC = -I ./includes/

NAME = webserv

CC = clang++

ifdef WITH_BONUS
	CFLAGS = -Wall -Wextra -Werror -std=c++98 -D BONUS ${INC}
else
	CFLAGS = -Wall -Wextra -Werror -std=c++98 ${INC}
endif

DCFLAGS = -g3 -fsanitize=address -D BONUS

${NAME} : $(SRCS)
	${CC} ${CFLAGS} ${SRCS} -o ${NAME}

test:
	${CC} ${DCFLAGS} ${SRCS} ${INC} -o ${NAME}
	rm -rf ./tests/put_test/file_should_exist_after
	./webserv configs/test.conf

test_hyeonski:
	${CC} ${DCFLAGS} ${SRCS} ${INC} -o ${NAME}
	rm -rf ./tests/put_test/file_should_exist_after
	rm -rf .res_*
	./webserv configs/test_hyeonski.conf

bonus:
	make WITH_BONUS=1 all

fclean:
	rm -rf ./tests/put_test/file_should_exist_after
	rm -rf ./tests/put_test/multiple_same
	rm -rf webserv.dSYM
	rm -rf ${NAME}

re: fclean all

all: ${NAME}

.PHONY: fclean re test
