
#include "Server.hpp"

void Server::run()
{
    while(true)
    {

    }
}



// https://www.geeksforgeeks.org/cpp/socket-programming-in-cpp/


void Server::setupSocket()
{
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(8080);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	listen(serverSocket, 5);
	int clientSocket = accept(serverSocket, nullptr, nullptr);
	char buffer[1024] = {0};
	recv(clientSocket, buffer, sizeof(buffer), 0);
	std::cout << "Message from client: " << buffer << std::endl;

	close(serverSocket);

}

void Server::socketClient()
{
	int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(8080);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

	const char* message = "Hello, server!";
	send(clientSocket, message, strlen(message), 0);

		close(clientSocket);
}