/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mayoub <mayoub@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/06/24 14:26:09 by sihemayoub        #+#    #+#             */
/*   Updated: 2023/07/09 15:14:18 by mayoub           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

std::string	readIndexFile()
{
	std::ifstream file("index.html");
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	//std::cout << "content : " << content << std::endl;
	return content;
}


void launch(Server* server)
{
	char		buff[30000];
	std::string	resp_header = "HTTP/1.1 200 OK\r\nServer: Apache/2.2.14 (Win32)\r\nContent-Type: text/html\r\nConnection: Closed\r\n";
	int			address_length = sizeof(server->address);

	int			new_socket;

	while (1)
	{
		std::cout << "====== WAIT ======" << std::endl;

		new_socket = accept(server->sockfd, (struct sockaddr *)&server->address, (socklen_t *)&address_length);
		std::cout << "buff : " << buff << std::endl;
		read(new_socket, buff, 30000);
		if (strncmp(buff, "GET / ", 6) == 0)
		{
			std::string indexContent = resp_header + "\r\n" + readIndexFile();
			std::cout << YELLOW << readIndexFile() << RESET << std::endl;
			// std::cout << "to send : " << resp_header << "\r\n" << indexContent << std::endl;
			write(new_socket, indexContent.c_str(), indexContent.length());
		}

// ! ******************************* IMG BIG BG ******************************************
		else if (strncmp(buff, "GET /ctardy.jpg ", 16) == 0)
		{
			std::ifstream imageFile("ctardy.jpg", std::ios::binary);
			if (!imageFile)
			{
				std::string errorResponse = "HTTP/1.1 404 Not Found\r\nServer: Apache/2.2.14 (Win32)\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>";
				write(new_socket, errorResponse.c_str(), errorResponse.length());
				continue;
			}

			std::string imageContent((std::istreambuf_iterator<char>(imageFile)), std::istreambuf_iterator<char>());
			imageFile.close();

			std::string imageHeader = "HTTP/1.1 200 OK\r\nServer: Apache/2.2.14 (Win32)\r\nContent-Type: image/jpeg\r\nContent-Length: " + std::to_string(imageContent.length()) + "\r\nConnection: Closed\r\n\r\n";

			write(new_socket, imageHeader.c_str(), imageHeader.length());
			write(new_socket, imageContent.c_str(), imageContent.length());
		}
		else if (strncmp(buff, "GET /noalexan.jpg ", 16) == 0)
		{
			std::ifstream imageFile("noalexan.jpg", std::ios::binary);
			if (!imageFile)
			{
				std::string errorResponse = "HTTP/1.1 404 Not Found\r\nServer: Apache/2.2.14 (Win32)\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>";
				write(new_socket, errorResponse.c_str(), errorResponse.length());
				continue;
			}

			std::string imageContent((std::istreambuf_iterator<char>(imageFile)), std::istreambuf_iterator<char>());
			imageFile.close();

			std::string imageHeader = "HTTP/1.1 200 OK\r\nServer: Apache/2.2.14 (Win32)\r\nContent-Type: image/jpeg\r\nContent-Length: " + std::to_string(imageContent.length()) + "\r\nConnection: Closed\r\n\r\n";

			write(new_socket, imageHeader.c_str(), imageHeader.length());
			write(new_socket, imageContent.c_str(), imageContent.length());
		}
		else if (strncmp(buff, "GET /mayoub.jpg ", 16) == 0)
		{
			std::ifstream imageFile("mayoub.jpg", std::ios::binary);
			if (!imageFile)
			{
				std::string errorResponse = "HTTP/1.1 404 Not Found\r\nServer: Apache/2.2.14 (Win32)\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>";
				write(new_socket, errorResponse.c_str(), errorResponse.length());
				continue;
			}

			std::string imageContent((std::istreambuf_iterator<char>(imageFile)), std::istreambuf_iterator<char>());
			imageFile.close();

			std::string imageHeader = "HTTP/1.1 200 OK\r\nServer: Apache/2.2.14 (Win32)\r\nContent-Type: image/jpeg\r\nContent-Length: " + std::to_string(imageContent.length()) + "\r\nConnection: charly je te mange tu vas rien faire\r\n\r\n";

			write(new_socket, imageHeader.c_str(), imageHeader.length());
			write(new_socket, imageContent.c_str(), imageContent.length());
		}
// ! *************************************************************************************

		else
		{
			std::string resp_header = "<html><body><h1>404 Not Found</h1></body></html>";
			write(new_socket, resp_header.c_str(), resp_header.length());
		}

		close(new_socket);
	}
}

int main(void)
{
	Server server = ServerConstruct(AF_INET, SOCK_STREAM, 0, INADDR_ANY, 8080, 10, launch);
	launch(&server);

	return (0);
}

// Version : 0.1.1
// Authors : Noah Alexandre, Marwan Ayoub, Charly Tardy
