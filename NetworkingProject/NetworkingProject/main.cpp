// You were working on the server receiving commands
// stopped cause it didn't get the commands from the buffer


#pragma comment(lib,"Ws2_32.lib")

#include "Definitions.h"
#include "Server.h"
#include "Client.h"

int main()
{
	WSADATA wsadata;
	WSAStartup(WINSOCK_VERSION, &wsadata);

	int choice;
	do
	{
		printf("Create a Server or Client?\n");
		printf("1) Server\n");
		printf("2) Client\n");
		std::cin >> choice;
	} while (choice != 1 && choice != 2);
	system("cls");
	if (choice == 1)
	{
		// Server Code
		Server server;

		// Create the listening socket and server address then bind them
		Server::ServerError result = server.CreateListenTcpSocket();
		if (result != Server::SUCCESS)
		{
			server.ShutdownServer();
			server.DisplayError(result);
			return WSACleanup();
		}

		// Create the UDP socket and options
		result = server.CreateUDPSocketBroadcast();
		if (result != Server::SUCCESS)
		{
			server.ShutdownServer();
			server.DisplayError(result);
			return WSACleanup();
		}

		// Accept incoming connects then start a receiving loop on a new thread
		result = server.AcceptIncomingConnections(server);
		if (result != Server::SUCCESS)
		{
			server.ShutdownServer();
			server.DisplayError(result);
			return WSACleanup();
		}

		server.ShutdownServer();
	}
	else if (choice == 2)
	{
		Client client;

		// Ask for ip and port
		Client::ClientError result = client.GetServerInfo();
		if (result != Client::SUCCESS)
		{
			client.DisconnectSocket();
			client.DisplayError(result);
			return WSACleanup();
		}
		system("cls");
		// create a tcp ipv4 socket
		result = client.CreateTcpSocket();
		if (result != Client::SUCCESS)
		{
			client.DisconnectSocket();
			client.DisplayError(result);
			return WSACleanup();
		}
		
		// create ipv4 address
		result = client.CreateAddrInfo();
		if (result != Client::SUCCESS)
		{
			client.DisconnectSocket();
			client.DisplayError(result);
			return WSACleanup();
		}

		// establish connection to server
		result = client.StartConnection();
		if (result != Client::SUCCESS)
		{
			client.DisconnectSocket();
			client.DisplayError(result);
			return WSACleanup();
		}

		// register the client with the server
		result = client.StartRegistration();
		if (result != Client::SUCCESS)
		{
			client.DisconnectSocket();
			client.DisplayError(result);
			return WSACleanup();
		}
		system("cls");
		// Start Receiver Thread
		client.CreateReceiveThread();

		// Start Transmit Loop
		result = client.TransmitLoop();
		if (result != Client::SUCCESS)
		{
			client.DisconnectSocket();
			client.DisplayError(result);
			return WSACleanup();
		}
	}
	system("pause");
	return WSACleanup();
}