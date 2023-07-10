/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mayoub <mayoub@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/06/24 13:38:06 by sihemayoub        #+#    #+#             */
/*   Updated: 2023/07/05 16:45:59 by mayoub           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# define ORANGE                      "\033[1;38;2;255;187;54m"
# define VIOLET                      "\033[1;38;2;158;46;156m"
# define BLUE                        "\033[1;38;2;73;153;254m"
# define RED                         "\033[1;38;2;255;0;0m"
# define GREEN                       "\033[1;38;2;69;176;26m"
# define YELLOW                      "\033[1;38;2;241;223;9m"
# define PINK                        "\033[1;38;2;240;170;223m"
# define CYAN                        "\033[1;38;2;43;236;195m"
# define JAUNE                       "\033[1;38;2;251;255;66"
# define RESET                       "\033[0m"
# define PORT                        8883
# define TRUE                        1
# define FALSE                       0
# define MAXLINE                     4096
# define MAX_EVENTS                  1024
# define RN                          "\r\n"
# define RNRN                        "\r\n\r\n"
# define TIME_OUT                    6000

# include <iostream>
# include <fstream>

# include <unistd.h>

# include <exception>
# include <stdexcept>

# include <sys/socket.h>
# include <netinet/in.h>

# define ERROR_EXIT 1
# define PUBLIC_DIR std::string("/Users/noahalexandre/Desktop/webserv/public")


typedef struct Server {

	int		domain;
	int		service;
	int		protocole;
	u_long	interface;
	int		port;
	int		backlog;

	struct sockaddr_in	address;

	int		sockfd;

	void	(*launch)(struct Server *server, char **);

}	Server;

struct Server	ServerConstruct( int domain, int service, int protocole, u_long interface, int port, int backlog, void (*launch)(struct Server *server, char **) );

#endif /* Server.hpp */
