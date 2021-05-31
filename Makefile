SRCNAME	=		\
					main.cpp\
					Client.cpp\
					Location.cpp\
					Manager.cpp\
					Server.cpp\
					Webserver.cpp\
					Request.cpp\
					Response.cpp\
					CGI.cpp

SRCDIR		=		./srcs/

SRCS		=		${addprefix ${SRCDIR}, ${SRCNAME}}

INC		=		-I ./includes/ -I /libft_cpp/

NAME		=		webserv

LIB_NAME	=		libft.a

CC			=		clang++

CF			=		-Wall -Wextra -Werror -std=c++98 ${INC} ${SRCS}
DCF			=		-g ${SRCS} -fsanitize=address

${NAME}     :
					make all -C "./libft_cpp"
					cp libft_cpp/${LIB_NAME} ${LIB_NAME}
					${CC} ${CF} ${LIB_NAME} ${INC} -o ${NAME} 

dbg		:
					${CC} ${DCF} ${LIB_NAME} -o ${NAME}
					lldb webserv -- configs/test.conf

test		:
					${CC} ${DCF} ${LIB_NAME} ${INC} -o ${NAME}
					rm -rf ./tests/put_test/file_should_exist_after
					rm -rf .res_*
					./webserv configs/test.conf &> test_result.txt

test_hyeonski:
					${CC} ${DCF} ${LIB_NAME} ${INC} -o ${NAME}
					rm -rf ./tests/put_test/file_should_exist_after
					rm -rf .res_*
					./webserv configs/test_hyeonski.conf

fclean		:
					make fclean -C "./libft_cpp"
					rm -rf webserv.dSYM
					rm -rf ${NAME}
					rm -rf ${LIB_NAME}

re			:		fclean all

all         :      	${NAME}

.PHONY		:		fclean re test
