# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: sihemayoub <sihemayoub@student.42.fr>      +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2023/07/27 22:43:15 by sihemayoub        #+#    #+#              #
#    Updated: 2023/07/27 22:45:50 by sihemayoub       ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CC:=gcc
CFLAGS:=-Werror -Wall -Wextra

NAME:=ft_ping

SRC:=$(addprefix src/, main.c utils.c request.c)
OBJ:=$(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(OBJ)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
