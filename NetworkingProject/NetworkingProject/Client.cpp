#include "Client.h"


// Client Love Letter Setup
Client::ClientError Client::GetServerInfo()
{
	BroadcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (BroadcastSocket == INVALID_SOCKET)
	{
		int ErrorCode = WSAGetLastError();
		if (ErrorCode == WSAECONNRESET) return DISCONNECT;
		else if (ErrorCode == WSAESHUTDOWN) return SHUTDOWN;
		else return UDP_SET_FAIL;
	}
	char optVal = '1'; // true
	int result = setsockopt(BroadcastSocket, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));
	if (result < 0) return UDP_SET_FAIL;
	int len = sizeof(struct sockaddr_in);
	broadcast_rec.sin_family = AF_INET;
	broadcast_rec.sin_port = htons(broadcast_port);
	broadcast_rec.sin_addr.s_addr = INADDR_ANY;
	result = bind(BroadcastSocket, (sockaddr*)&broadcast_rec, sizeof(broadcast_rec));
	if (result < 0) return UDP_BIND_FAIL;
	char recBuff[50];
	int buffLen = 50;

	printf("Waiting to receive server information\n");
	// we got a broadcast from server
	result = recvfrom(BroadcastSocket, recBuff, buffLen, 0, (sockaddr*)&broadcast_trans, &len);
	if (result == -1) return BROADCAST_FAIL;

	bool isAddressDone = false;
	string sPort;
	for (int i = 0; i < result; ++i)
	{
		if (recBuff[i] == '|')
		{
			isAddressDone = true;
			i++;
		}

		if (!isAddressDone)
			addy = addy + recBuff[i];
		else sPort = sPort + recBuff[i];
	}
	int iPort;
	sscanf_s(sPort.data(), "%d", &iPort);
	port = iPort;
	printf("Server information received\n");
	return SUCCESS;
	//// Ask for ip and port
	//printf("Enter the Server's IP address. Ex: 127.0.0.1\nServer IP: ");
	//std::cin >> address;
	//printf("\nEnter the Port number. Ex: 31337\nServer Port: ");
	//std::cin >> port;
}
Client::ClientError Client::CreateTcpSocket()
{
	printf("Starting Client...\n");
	// create the client communication socket using ipv4 tcp
	socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketFD == INVALID_SOCKET)
	{
		int ErrorCode = WSAGetLastError();
		if (ErrorCode == WSAECONNRESET) return DISCONNECT;
		else if (ErrorCode == WSAESHUTDOWN) return SHUTDOWN;
		else return SOCKET_FAIL;
	}
	return SUCCESS;
}
Client::ClientError Client::CreateAddrInfo()
{
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	int addressResult = inet_pton(AF_INET, addy.c_str(), &server_address.sin_addr);
	if (addressResult <= 0) return ADDRESS_FAIL;

	return SUCCESS;
}
Client::ClientError Client::StartConnection()
{
	// connect to the server
	int conResult = connect(socketFD, (struct sockaddr*)&server_address, sizeof(server_address));
	if (conResult == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) return CONNECTION_FAIL;
	printf("Server found.\n");
	return SUCCESS;
}
Client::ClientError Client::StartRegistration()
{
	// get a username from the user
	string name;
	printf("\n\nEnter a username: ");
	cin >> name;
	username = name;
	// send the username to the server
	printf("\nRegistering with server.\n");
	string msg = "$register ";
	msg.append(username);
	ClientError result = Transmit(msg);
	if (result != SUCCESS) return result;
	// server should reply: "SV_SUCCESS" if there is room or "SV_FULL" if there is no room
	string readBuffer;
	char buffer[BUFFER_SIZE];
	unsigned read_size = 0;
	result = Receive(buffer, BUFFER_SIZE, read_size);
	if (result != SUCCESS) return result;
	for (int i = 0; i < read_size; ++i)
	{
		if (buffer[i] == '\0') break;
		readBuffer += buffer[i];
	}
	if (readBuffer.substr(0, 10) == "SV_SUCCESS")
	{
		printf("Registration completed.\n");
		// tell the server we are ready to start looping
		msg = "CL_READY";
		ClientError result = Transmit(msg);
		if (result != SUCCESS) return result;
		return SUCCESS;
	}
	else if (readBuffer.find("SV_FULL") != string::npos) return SERVER_FULL; // server doesnt have room for us
	else return REGISTRATION_FAIL; // server done goofed and sent the wrong reply
}

// client receiving love letter functions
int Client::tcp_recv_whole(SOCKET s, char* buf, int len)
{
	int total = 0;

	do
	{
		int ret = recv(s, buf + total, len - total, 0);
		if (ret < 1)
			return ret;
		else
			total += ret;

	} while (total < len);

	return total;
}
Client::ClientError Client::Receive(char* buffer, int32_t size, unsigned& msg_size)
{
	uint8_t size_ = 0;
	int recResult = tcp_recv_whole(socketFD, (char*)&size_, 1);
	if (recResult == SOCKET_ERROR)
	{
		int ErrorCode = WSAGetLastError();
		if (ErrorCode == WSAECONNRESET) return DISCONNECT;
		else if (ErrorCode == WSAESHUTDOWN) return SHUTDOWN;
		else return DISCONNECT;
	}
	msg_size = size_;
	recResult = tcp_recv_whole(socketFD, buffer, size_);
	if (recResult == SOCKET_ERROR)
	{
		int ErrorCode = WSAGetLastError();
		if (ErrorCode == WSAECONNRESET) return DISCONNECT;
		else if (ErrorCode == WSAESHUTDOWN) return SHUTDOWN;
		else return DISCONNECT;
	}
	else if (recResult > size) return PARAM_FAIL;

	return SUCCESS;
}
void Client::ReceiveLoop()
{
	ClientError result; // enum that I use for error checking
	while (isRunning) // only run when no errors
	{
		string readBuffer; // string container for message
		char msg[BUFFER_SIZE]; // char container for message
		unsigned msg_size = 0; // how many chars are in the message

		result = Receive(msg, BUFFER_SIZE, msg_size); // receives a message and stores it inside msg. Also updates msg_size to how many chars were received
		if (result != SUCCESS) break; // did we die?

		for (int i = 0; i < msg_size; ++i) // this is just my way of converting a char array to a string
		{
			if (msg[i] == '\0') break;
			readBuffer += msg[i];
		}

		string first_three; // I want the first 3 chars of the msg
		for (int i = 0; i < 3; ++i)
			first_three += readBuffer.c_str()[i];

		// check if the message is a server msg "SV_"
		if (first_three != "SV_") PrintToScreen(readBuffer, true); // if it's not a server message, then print out whatever msg we got from the server
		else
		{
			// we got a server message so handle it
			result = ProcessServerMessage(readBuffer);
			if (result != SUCCESS) isRunning = false; // did we die?
		}
	}

	// Socket is no longer receiving so shut it down
	if (socketFD != INVALID_SOCKET)
	{
		int shutResult = shutdown(socketFD, SD_SEND);
		int closeResult = closesocket(socketFD);
		socketFD = INVALID_SOCKET;
		DisplayError(result);
	}
}
Client::ClientError Client::ProcessServerMessage(string message)
{
	ClientError result = SUCCESS;
	if (message == SV_REGISTER) // server sends this when client sends a love letter and the client isnt registered
	{
		// tell client to register
		string message = "You must register before sending a message.";
		PrintToScreen(message, false);
	}
	else if (message == SV_HELP) // server sends this to client when client wants to see the command list
	{
		// tell the client each command they can enter
		PrintToScreen(COMMANDS, false);
	}
	else if (message == SV_LOG) // server sends this when it's going to send log data
	{
		// read in how many lines we are going to take. it'll always be one line at least
		int line_count = 0;
		string readBuffer;
		char msg[BUFFER_SIZE];
		unsigned msg_size = 0;
		result = Receive(msg, BUFFER_SIZE, msg_size);
		if (result != SUCCESS) return result;
		for (int i = 0; i < msg_size; ++i)
		{
			if (msg[i] == '\0') break;
			readBuffer += msg[i];
		}
		line_count = stoi(readBuffer);
		// now we have the line count so create or replace the log file and write each line to it
		int lines = 0;
		fstream log;
		log.open("ClientLog.txt", fstream::in | fstream::out | fstream::trunc);
		log << ""; // if ClientLog does exist we empty the file
		log.close();
		log.open("ClientLog.txt", std::fstream::in | std::fstream::out | std::fstream::app);
		while (true)
		{
			string line;
			char line_char[BUFFER_SIZE];
			unsigned line_size = 0;

			result = Receive(line_char, BUFFER_SIZE, line_size);
			if (result != SUCCESS) return result;

			for (int i = 0; i < line_size; ++i)
			{
				if (line_char[i] == '\0') break;
				line += line_char[i];
			}

			if (line == SV_LOG_END) break;
			else
			{
				log << line << "\n";
			}
		}
		log.close();
		PrintToScreen("Logged saved as ClientLog.txt", false);
	}
	else if (message == SV_EMPTY) // server sends this when getlist has no users
	{
		// tell client user list was empty
		string message = "User list was empty.";
		PrintToScreen(message, false);
	}
	else if (message == SV_NOT_COMMAND) // server sends this when client sends an unknown command
	{
		// tell client they entered a wrong command and to use $help to see command list
		string message = "Unknown command entered. User $help to see command list.";
		PrintToScreen(message, false);
	}
	else if (message == SV_ALR_REGISTERED) // server sends this when client uses $register when they are already registered
	{
		// tell client they are already registered
		string message = "You are already registered.";
		PrintToScreen(message, false);
	}
	else if (message == SV_EXIT)
	{
		DisconnectSocket();
		system("cls");
		string message = "Successfully left the chat room.";
		cout << message << std::endl;
		system("pause");
		exit(0);
		return STOP_CLIENT;
	}
	return result;
}
void Client::CreateReceiveThread()
{
	thread receive_thread(&Client::ReceiveLoop, this);
	receive_thread.detach();
}

// client sending love letter functions
int Client::tcp_send_whole(SOCKET skSocket, const char* data, uint16_t length)
{
	int result;
	int bytesSent = 0;

	while (bytesSent < length)
	{
		result = send(skSocket, (const char*)data + bytesSent, length - bytesSent, 0);

		if (result <= 0)
			return result;

		bytesSent += result;
	}

	return bytesSent;
}
Client::ClientError Client::Transmit(char* data, int32_t length)
{
	if (length < 0 || length > BUFFER_SIZE) return PARAM_FAIL;

	int sendResult = tcp_send_whole(socketFD, (char*)&length, 1);
	if ((sendResult == SOCKET_ERROR) || (sendResult == 0))
	{
		int error = WSAGetLastError();
		if (error == WSAESHUTDOWN) return SHUTDOWN;
		else return DISCONNECT;
	}
	sendResult = tcp_send_whole(socketFD, data, length);
	if ((sendResult == SOCKET_ERROR) || (sendResult == 0))
	{
		int error = WSAGetLastError();
		if (error == WSAESHUTDOWN) return SHUTDOWN;
		else return DISCONNECT;
	}
	return SUCCESS;
}
Client::ClientError Client::Transmit(string message)
{
	int love_letter_size = message.size();
	char* temp_love_letter = new char[love_letter_size];
	for (int i = 0; i < love_letter_size; ++i)
	{
		if (message[i] == '\0') break;
		temp_love_letter[i] = message[i];
	}
	ClientError result = Transmit(temp_love_letter, love_letter_size);
	delete[] temp_love_letter;
	return result;
}
Client::ClientError Client::TransmitLoop()
{
	while (isRunning)
	{
		string message;

		// write out the love letter
		//printf("\nMessage: ");
		getline(cin >> ws, message);

		// send the server the love letter
		ClientError result = Transmit(message);
		if (result != SUCCESS) return result;
		PrintToScreen("", true);
	}
	return STOP_CLIENT;
}

// util functions
void Client::DisconnectSocket()
{
	int shutResult = shutdown(socketFD, SD_SEND);
	int closeResult = closesocket(socketFD);
	socketFD = INVALID_SOCKET;
}
bool Client::isWhiteSpace(unsigned char c)
{
	if (c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') return true;
	else return false;
}
void Client::DisplayError(ClientError error)
{
	switch (error)
	{
	case SUCCESS:
	{
		// yay it worked
	} break;
	case DISCONNECT:
	{
		printf("\n\nError from WSAGetLastError() -> Socket Disconnect\n\n");
	} break;
	case SHUTDOWN:
	{
		printf("\n\nError from WSAGetLastError() -> Socket Shutdown\n\n");
	} break;
	case SOCKET_FAIL:
	{
		printf("\n\nError from socket() -> Socket Error\n\n");
	} break;
	case ADDRESS_FAIL:
	{
		printf("\n\nError from inet_pton() -> Address Error\n\n");
	} break;
	case CONNECTION_FAIL:
	{
		printf("\n\nError from connect() -> Connection Error\n\n");
	} break;
	case REGISTRATION_FAIL:
	{
		printf("\n\nError from receiving wrong server capacity info during registration -> Registration Error\n\n");
	} break;
	case SERVER_FULL:
	{
		printf("\n\nServer is at max capacity.\n\n");
	}
	case UDP_SET_FAIL:
	{
		printf("\n\nError setting up UDP socket.\n\n");
	} break;
	case UDP_BIND_FAIL:
	{
		printf("\n\nError binding the UDP socket.\n\n");
	} break;
	case BROADCAST_FAIL:
	{
		printf("\n\nError receiving broadcast.\n\n");
	} break;
	case SELECT_FAIL:
	{
		printf("\n\nSelect returned INVALID_SOCKET.\n\n");
	} break;
	case INFO_FAIL:
	{
		printf("\n\nFailed to get information from broadcast\n\n");
	} break;
	default:
	{
		printf("\n\nClient::DisplayError hit default switch -> Unknown Error\n\n");
	} break;
	}
}
void Client::PrintToScreen(string message, bool isClientMessage)
{
	// clear the line
	printf("\r"); // cursor to the start of line
	printf(" "); // blank the line
	printf("\r"); // put the cursor back again
	if (!isClientMessage)
	{
		string serverMsg = "[Server]: ";
		serverMsg.append(message);
		printf("%s\n", serverMsg.c_str());
	}
	else if (message != "") printf("%s\n", message.c_str()); // print the message if it contains anything
	string print = "[";
	print.append(username);
	print.append("]: ");
	printf("%s", print.c_str()); // add text for us sending
}


