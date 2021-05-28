#ifndef TYPE_HPP
# define TYPE_HPP

# define BUFFER_SIZE 65536

typedef enum			e_status
{
	REQUEST_RECEIVING,
	REQUEST_COMPLETE, // 구분할 필요 생기면 그 때 함
	RESPONSE_COMPLETE,
}						t_status;

# define MANAGER Manager::getInstance()

#endif
