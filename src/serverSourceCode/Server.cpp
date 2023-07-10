/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mayoub <mayoub@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/06/24 13:37:59 by sihemayoub        #+#    #+#             */
/*   Updated: 2023/07/03 13:34:08 by mayoub           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

struct Server	ServerConstruct( int domain, int service, int protocole, u_long interface, int port, int backlog, void (*launch)(struct Server *server, char **) ) {

	struct Server	server;

	server.domain = domain;
	server.service = service;
	server.protocole = protocole;
	server.interface = interface;
	server.port = port;
	server.backlog = backlog;

	server.address.sin_family = domain;
	server.address.sin_port = htons(port);
	server.address.sin_addr.s_addr = htonl(interface);

	server.sockfd = socket(domain, service, protocole);
	if (server.sockfd <= 0) {
		std::cerr << "Socket error" << std::endl;
		exit(ERROR_EXIT);
	}

	int on = 1;
	if (setsockopt(server.sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) == -1) {
		std::cerr << "setsockopt error" << std::endl;
		exit(ERROR_EXIT);
		// throw std::runtime_error("setsockopt failed");
	}

	int bind_value = bind(server.sockfd, (const struct sockaddr *)&server.address, sizeof(server.address));
	if (bind_value < 0) {
		std::cerr << "Bind error : " << bind_value << std::endl;
		exit(ERROR_EXIT);
	}

	int listen_value = listen(server.sockfd, server.backlog);
	if (listen_value < 0) {
		std::cerr << "Listen error : " << listen_value << std::endl;
		exit(ERROR_EXIT);
	}

	server.launch = launch;

	return (server);

}
