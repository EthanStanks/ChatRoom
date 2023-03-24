#pragma once
#include "Definitions.h"

class Client
{

public:
	enum ClientError
	{
		SUCCESS = 0, DISCONNECT = 1, SHUTDOWN = 2,
		SOCKET_FAIL = 3, ADDRESS_FAIL = 4, CONNECTION_FAIL = 5,
		PARAM_FAIL = 6, SERVER_FULL = 7, REGISTRATION_FAIL = 8, STOP_CLIENT = 9,
		UDP_SET_FAIL = 10, UDP_BIND_FAIL = 11, BROADCAST_FAIL = 12, SELECT_FAIL = 13, INFO_FAIL = 14
	};
	// Client Love Letter Setup
	ClientError GetServerInfo();
	ClientError CreateTcpSocket();
	ClientError CreateAddrInfo();
	ClientError StartConnection();
	ClientError StartRegistration();

	// public client receiving love letter functions
	void ReceiveLoop();
	void CreateReceiveThread();

	// public client sending love letter functions
	ClientError TransmitLoop();

	// public util functions
	void DisconnectSocket();
	void DisplayError(ClientError error);

private:
	int socketFD = INVALID_SOCKET; // clients communication socket
	SOCKET BroadcastSocket;
	struct sockaddr_in server_address; // server info
	struct sockaddr_in broadcast_rec;
	struct sockaddr_in broadcast_trans;
	uint16_t port; // port for connection
	uint16_t broadcast_port = 31338; // port for connection
	string addy;
	string username;
	bool isRunning = true;

	// private client receiving love letter functions
	int tcp_recv_whole(SOCKET s, char* buf, int len);
	//ClientError Receive(string& buffer, int32_t size);
	ClientError Receive(char* buffer, int32_t size, unsigned& msg_size);
	ClientError ProcessServerMessage(string message);

	// private client sending love letter functions
	int tcp_send_whole(SOCKET skSocket, const char* data, uint16_t length);
	ClientError Transmit(char* data, int32_t length);
	ClientError Transmit(string message);

	// private util functions
	bool isWhiteSpace(unsigned char c);
	void PrintToScreen(string message, bool isClientMessage);

};

