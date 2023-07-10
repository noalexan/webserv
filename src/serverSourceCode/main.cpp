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
	std::ifstream file("/Users/noahalexandre/Desktop/webserv/public/index.html");
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return content;
}

void launch(Server* server, char** env)
{
	std::string	resp_header = "HTTP/1.1 200 OK\r\nServer: Apache/2.2.14 (Win32)\r\nContent-Type: text/html\r\nConnection: Closed\r\n";
	int			address_length = sizeof(server->address);


	while (true)
	{

		std::cout << "====== WAIT ======" << std::endl;
		int new_socket = accept(server->sockfd, (struct sockaddr *)&server->address, (socklen_t *)&address_length);

		if (new_socket < 0) {
			std::cerr << "accept error" << std::endl;
			exit(1);
		}

		std::cout << "====== ACCEPT ======" << std::endl;

		std::string request;
		char buff[3000];
		read(new_socket, buff, 3000);
		request = buff;

		std::cout << std::endl << request;

		if (request.find("GET / ") != std::string::npos || request.find("GET /index.html ") != std::string::npos) {
			std::string indexContent = resp_header + "\r\n" + readIndexFile();
			write(new_socket, indexContent.c_str(), indexContent.length());
		}

		else if (request.find("GET /internet.ico ") != std::string::npos)
		{
			std::ifstream imageFile("/Users/noahalexandre/Desktop/webserv/public/internet.ico", std::ios::binary);
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

		else if (request.find("GET /phpinfo.php ") != std::string::npos)
		{
			int pid = fork();

			if (!pid) {
				dup2(new_socket, 1);
				close(new_socket);

				write(1, "HTTP/1.1 200 OK\r\n", 17);

				char *argv[] = {"/usr/local/bin/php-cgi", "/Users/noahalexandre/Desktop/webserv/public/phpinfo.php", NULL};

				execve("/usr/local/bin/php-cgi", argv, env);

				std::cerr << "execve error" << std::endl;
				exit(1);
			}
		}

		else if (request.find("GET /helloworld.py ") != std::string::npos)
		{
			int pid = fork();

			if (!pid) {
				dup2(new_socket, 1);
				close(new_socket);

				write(1, "HTTP/1.1 200 OK\r\n", 17);

				char *argv[] = {"/usr/local/bin/python3", "/Users/noahalexandre/Desktop/webserv/public/helloworld.py", NULL};

				execve("/usr/local/bin/python3", argv, env);

				std::cerr << "execve error" << std::endl;
				exit(1);
			}
		}

// ! ******************************* IMG BIG BG ******************************************
		else if (request.find("GET /ctardy.jpg ") != std::string::npos)
		{
			std::ifstream imageFile("/Users/noahalexandre/Desktop/webserv/public/ctardy.jpg", std::ios::binary);
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
		else if (request.find("GET /noalexan.jpg ") != std::string::npos)
		{
			std::ifstream imageFile("/Users/noahalexandre/Desktop/webserv/public/noalexan.jpg", std::ios::binary);
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
		else if (request.find("GET /mayoub.jpg ") != std::string::npos)
		{
			std::ifstream imageFile("/Users/noahalexandre/Desktop/webserv/public/mayoub.jpg", std::ios::binary);
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

		else write(new_socket, "HTTP/1.1 404 Not Found\r\nServer: Apache/2.2.14 (Win32)\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n<html><body><h1>404 Not Found</h1><a href=/>back to home</a></body></html>", 176);

		close(new_socket);
	}
}

int main(int, char **, char **env)
{
	Server server = ServerConstruct(AF_INET, SOCK_STREAM, 0, INADDR_ANY, 80, 10, launch);
	launch(&server, env);
	return (0);
}

// Version : 0.1.1
// Authors : Noah Alexandre, Marwan Ayoub, Charly Tardy
