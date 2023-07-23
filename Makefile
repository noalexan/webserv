CC=c++
CXXFLAGS=-Werror -Wextra -Wall -std=c++98 -Isrc

NAME=webserv

SRC=$(addprefix src/, main.cpp Config/Config.cpp Request/Request.cpp Response/Response.cpp)
OBJ=$(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
