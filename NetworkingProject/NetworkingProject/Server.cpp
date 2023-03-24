#include "Server.h"


// Server setup
Server::ServerError Server::CreateListenTcpSocket()
{
	//Ask for ipand port
	printf("Enter the Server's IP address. Ex: 127.0.0.1\nServer IP: ");
	std::cin >> address;
	printf("\nEnter the Port number. Ex: 31337\nServer Port: ");
	std::cin >> port;
	system("cls");
	printf("Starting server.\n");
	// create the socket
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) return SOCKET_FAIL;
	// bind the ip and port to the socket
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	int addressResult = inet_pton(AF_INET, address.c_str(), &server_address.sin_addr);
	if (addressResult <= 0) return ADDRESS_FAIL;

	int bindResult = bind(ListenSocket, (sockaddr*)&server_address, sizeof(server_address));
	if (bindResult == SOCKET_ERROR)
	{
		int ErrorCode = WSAGetLastError();
		if (ErrorCode == WSAECONNRESET) return DISCONNECT;
		else if (ErrorCode == WSAESHUTDOWN) return SHUTDOWN;
		else return BIND_FAIL;
	}
	printf("Server Started.\n");
	return SUCCESS;
}
Server::ServerError Server::CreateUDPSocketBroadcast()
{
	BroadcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (BroadcastSocket == INVALID_SOCKET) return UDP_SETUP_FAIL;
	char optVal = '1'; // true
	int result = setsockopt(BroadcastSocket, SOL_SOCKET, SO_BROADCAST, &optVal, sizeof(optVal));
	if (result < 0) return UDP_SETUP_FAIL;

	broadcast_address.sin_family = AF_INET;
	broadcast_address.sin_port = htons(broadcast_port);
	broadcast_address.sin_addr.s_addr = INADDR_BROADCAST; // 255.255.255.255
	return SUCCESS;
}
Server::ServerError Server::AcceptIncomingConnections(Server& server_instance)
{
	int lisResult = listen(ListenSocket, SOMAXCONN); // tells winsock this socket is meant for listening with a queue size of max allowed
	if (lisResult == SOCKET_ERROR)
	{
		int ErrorCode = WSAGetLastError();
		if (ErrorCode == WSAECONNRESET) return DISCONNECT;
		else if (ErrorCode == WSAESHUTDOWN) return SHUTDOWN;
		else return LISTEN_FAIL;
	}
	CreateLogFile(); // create the log file if it doesnt exist
	FD_ZERO(&master); // zero the master set
	FD_SET(ListenSocket, &master); // add our listening socket to the set
	timeval timeout = { 1, 0 };
	while (true)
	{
		if (_kbhit())
		{
			if (_getch() == '\b') return SERVER_SHUTDOWN; // if backspace is hit, server will shutdown
		}
		ready = master;
		// See who is sending love letters
		int socketCount = select(0, &ready, NULL, NULL, &timeout);

		if (socketCount == INVALID_SOCKET) // select failed
		{
			int ErrorCode = WSAGetLastError();
			if (ErrorCode == WSAECONNRESET) return DISCONNECT;
			else if (ErrorCode == WSAESHUTDOWN) return SHUTDOWN;
			else return SELECT_FAIL;
		}
		else if (socketCount == 0) // select timed out
		{
			// broadcast server information
			ServerError result = BroadcastInformation();
			if (result != SUCCESS) return BROADCAST_FAIL;
		}
		else // select put something in the ready set
		{
			// check to see if there is any new connection
			if (FD_ISSET(ListenSocket, &ready))
			{
				// variables we will need if accept works
				SOCKET accept_socket;
				struct sockaddr_in client_address;
				int client_addr_size = sizeof(struct sockaddr_in);
				// accept the connection
				accept_socket = accept(ListenSocket, (sockaddr*)&client_address, &client_addr_size);
				if (accept_socket == INVALID_SOCKET)
				{
					int ErrorCode = WSAGetLastError();
					if (ErrorCode == WSAECONNRESET) return DISCONNECT;
					else if (ErrorCode == WSAESHUTDOWN) return SHUTDOWN;
					else return ACCEPT_FAIL;
				}
				// accept didnt have any issues so let's make this new connection a user
				User NewUser;
				NewUser.address = client_address;
				NewUser.CommunicationSocket = accept_socket;
				NewUser.isRegistered = false;
				NewUser.Username = "UnregisteredUser";
				users_not_registered.push_back(NewUser);
				// also add the socket to the master set
				FD_SET(accept_socket, &master);
			}

			// loop through all possible connections
			for (int i = 0; i < socketCount; ++i)
			{
				SOCKET current_sock = ready.fd_array[i]; // current socket from the ready fd_set
				if (current_sock == ListenSocket) continue; // if this is our listen socket then dont do anything, we handled new connections above
				else
				{
					// must find the user that the current_sock belongs to
					User current_user; // will be a registered or unregistered user
					for (int i = 0; i < users_registered.size(); ++i) // check if the current_sock belongs to a registered user
					{
						if (users_registered[i].CommunicationSocket == current_sock) // we found our user
							current_user = users_registered[i];
					}
					if (current_user.CommunicationSocket != current_sock) // if we didnt find a registered user then see if we find an unregistered user
					{
						for (int i = 0; i < users_not_registered.size(); ++i) // check if the current_sock belongs to an unregistered user
						{
							if (users_not_registered[i].CommunicationSocket == current_sock) // we found our user
								current_user = users_not_registered[i];
						}
						if (current_user.CommunicationSocket != current_sock) return FIND_USER_FAIL; // if we still didnt find a user then there was an issue
					}
					ServerError result = ReceiveMessage(current_user); // receive a love letter
					if (result != SUCCESS && result != DISCONNECT) return result; // no more love letters if there was an issue
					else if (result == DISCONNECT) ClientDisconnect(current_user); // client did an ungraceful disconnect so boot them
				}
			}
		}
	}
}

// Server love letter sending
Server::ServerError Server::BroadcastInformation()
{
	string message = address;
	message.append("|");
	message.append(to_string(port));

	int love_letter_size = message.size();
	char* temp_love_letter = new char[love_letter_size];
	for (int i = 0; i < love_letter_size; ++i)
	{
		if (message[i] == '\0') break;
		temp_love_letter[i] = message[i];
	}
	int result = sendto(BroadcastSocket, temp_love_letter, love_letter_size, 0, (sockaddr*)&broadcast_address, sizeof(broadcast_address));
	if (result == -1) return BROADCAST_FAIL;
	//printf("Broadcasted: %s", message.c_str());
	delete[] temp_love_letter;
	return SUCCESS;
}
int Server::tcp_send_whole(SOCKET skSocket, const char* data, uint16_t length)
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
Server::ServerError Server::sendMessage(SOCKET& ComSocket, char* data, int32_t length)
{
	if (length < 0 || length > BUFFER_SIZE) return PARAM_FAIL;

	int sendResult = tcp_send_whole(ComSocket, (char*)&length, 1);
	if ((sendResult == SOCKET_ERROR) || (sendResult == 0))
	{
		/*int error = WSAGetLastError();
		if (isServerActive) return SHUTDOWN;
		else return DISCONNECT;*/
		int ErrorCode = WSAGetLastError();
		if (ErrorCode == WSAECONNRESET) return DISCONNECT;
		else if (ErrorCode == WSAESHUTDOWN) return SHUTDOWN;
		else return DISCONNECT;
	}
	sendResult = tcp_send_whole(ComSocket, data, length);
	if ((sendResult == SOCKET_ERROR) || (sendResult == 0))
	{
		int ErrorCode = WSAGetLastError();
		if (ErrorCode == WSAECONNRESET) return DISCONNECT;
		else if (ErrorCode == WSAESHUTDOWN) return SHUTDOWN;
		else return DISCONNECT;
	}
	return SUCCESS;
}
Server::ServerError Server::sendMessage(User& user, string message, bool logMessage)
{
	int love_letter_size = message.size();
	char* temp_love_letter = new char[love_letter_size];
	//memset(temp_love_letter, 0, love_letter_size);
	for (int i = 0; i < love_letter_size; ++i)
	{
		if (message[i] == '\0') break;
		temp_love_letter[i] = message[i];
	}
	ServerError result = sendMessage(user.CommunicationSocket, temp_love_letter, love_letter_size);
	if (logMessage)
	{
		RemoveWordFromLine(message, "[Server]: ");
		LogMessage("Server", message);
	}
	delete[] temp_love_letter;
	return result;
}
Server::ServerError Server::EchoToOthers(User user, string buffer)
{
	for (int i = 0; i < users_registered.size(); ++i)
	{
		User current_user = users_registered[i];
		if (current_user.isRegistered && current_user.CommunicationSocket != user.CommunicationSocket)
		{
			ServerError result = sendMessage(current_user, buffer, false);
			if (result != SUCCESS) return result;
		}
	}
	return SUCCESS;
}
Server::ServerError Server::EchoToOthers(string buffer)
{
	for (int i = 0; i < users_registered.size(); ++i)
	{
		User current_user = users_registered[i];
		if (current_user.isRegistered)
		{
			ServerError result = sendMessage(current_user, buffer, false);
			if (result != SUCCESS) return result;
		}
	}
	return SUCCESS;
}
Server::ServerError Server::EchoPreset(User& user, int preset)
{
	string echo = "";
	switch (preset)
	{
	default:
	{
		echo = SV_RECEIVED; // default echo
	} break;
	case 0:
	{
		echo = SV_RECEIVED; // tell the client we received it's love letter
	} break;
	case 1:
	{
		echo = SV_SUCCESS; // tell the client registration was a success
	} break;
	case 2:
	{
		echo = SV_FULL; // tell the client there is no room for it on the server
	} break;
	case 3:
	{
		echo = SV_REGISTER; // tell the client it needs to register with the server
	} break;
	case 4:
	{
		echo = SV_HELP; // tell the client to print out the help information
	} break;
	case 5:
	{
		echo = SV_LOG; // tell the client to get ready for reading log data
	} break;
	case 6:
	{
		echo = SV_EMPTY; // tell the client the userlist is empty
	}break;
	case 7:
	{
		echo = SV_NOT_COMMAND; // tell the client the command they entered was not found
	}break;
	case 8:
	{
		echo = SV_NOT_MESSAGE; // tell the client the CL_ love letter they entered was not found
	}break;
	case 9:
	{
		echo = SV_ALR_REGISTERED; // tell the client that they have already registered with the server
	} break;
	case 10:
	{
		echo = SV_EXIT;
	} break;
	case 11:
	{
		echo = SV_LOG_END;
	} break;
	}
	ServerError result = sendMessage(user, echo, true);
	return result;
}
Server::ServerError Server::Echo(User& user, string msg)
{
	return sendMessage(user, msg, true);
}
Server::ServerError Server::EchoChatMessage(User& user, string msg)
{
	// add the user's name before their love letter
	string echo = "[";
	echo.append(user.Username);
	echo.append("]: ");
	echo.append(msg.c_str());
	msg = echo;
	// now echo the love letter to each client
	ServerError result = EchoToOthers(user, echo);
	return result;
}

// Server love letter receiving
int Server::tcp_recv_whole(SOCKET s, char* buf, int len)
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
//Server::ServerError Server::readMessage(SOCKET& ComSocket, string* buffer, int32_t size)
Server::ServerError Server::readMessage(SOCKET& ComSocket, char* buffer, int32_t size, unsigned& read_size)
{
	uint8_t size_ = 0;
	int recResult = tcp_recv_whole(ComSocket, (char*)&size_, 1);
	if (recResult == SOCKET_ERROR)
	{
		int ErrorCode = WSAGetLastError();
		if (ErrorCode == WSAECONNRESET) return DISCONNECT;
		else if (ErrorCode == WSAESHUTDOWN) return SHUTDOWN;
		else return DISCONNECT;
	}
	read_size = size_;
	recResult = tcp_recv_whole(ComSocket, buffer, size_);
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
Server::ServerError Server::ReceiveMessage(User& user)
{
	string readMsg; // this will hold our love message
	//readMsg[0] = '\0'; // no idea why I have this
	char buffer[BUFFER_SIZE];
	unsigned read_size = 0;
	ServerError result = readMessage(user.CommunicationSocket, buffer, BUFFER_SIZE, read_size); // read in our love letter
	if (result != SUCCESS) return result; // did reading cause any issues?
	for (int i = 0; i < read_size; ++i)
	{
		if (buffer[i] == '\0') break;
		readMsg += buffer[i];
	}
	// TODO write readMsg to server log.txt
	LogMessage(user.Username, readMsg);


	result = ProcessMessage(user, readMsg); // now that we have a love letter, let's check what kind of love letter it is
	return result; // tell HQ how we are doing with handling love letters
}
Server::ServerError Server::ProcessMessage(User& user, string msg)
{
	ServerError result;
	string echo = "";

	string first_three; // I want the first 3 chars of the msg
	for (int i = 0; i < 3; ++i)
		first_three += msg.c_str()[i];

	if (first_three[0] == '$') // client sent a command love letter
	{
		result = CommandReceived(user, msg, BUFFER_SIZE); // reads the command love letters and respond
		if (result != SUCCESS) return result;
	}
	else if (first_three == "CL_") // client sent an echo love letter back
	{
		result = CL_Received(user, msg, BUFFER_SIZE); // reads the CL_ love letter and responds if needed
		if (result != SUCCESS) return result;
	}
	else if (user.isRegistered) // client sent us a normal love letter
	{
		result = EchoChatMessage(user, msg); // send the love letter to all clients
		if (result != SUCCESS) return result;
		string print = "[";
		print.append(user.Username);
		print.append("]: ");
		print.append(msg);
		printf("%s\n", print.c_str()); // print the love letter to the server's chat
	}
	else result = EchoPreset(user, 3); // client is trying to send a fake love letter so tell them to register

	return result;
}

// Server love letter commands
Server::ServerError Server::CommandReceived(User& user, string buffer, int bufferSize)
{
	Command UserCommand = CommandData(buffer.c_str(), bufferSize); // break the love letter up so the server knows the command and what is typed after

	CommandResult commandResult = CommandProcess(UserCommand.command, user, UserCommand.input); // complete the logic for the command that was entered

	ServerError result = CommandResponse(user, commandResult); // echo back the result to the client after doing the command

	return result;
}
Server::Command Server::CommandData(const char* buffer, int bufferSize)
{
	string command;
	string input;
	bool isCommand = true;
	// get the command they put in
	int i = 0;
	while (true)
	{
		if (isCommand) command += buffer[i];
		else input += buffer[i];
		++i;
		if (buffer[i] == '\0') break;
		if (buffer[i] == ' ' && isCommand)
		{
			isCommand = false;
			++i;
		}
		if (i >= bufferSize) break;
	}
	// buffer contained "$register Username"
	Command UserCommand;
	UserCommand.command = command; // $register
	UserCommand.input = input;    // Username
	return UserCommand;
}
Server::CommandResult Server::CommandProcess(string command, User& user, string username)
{
	CommandResult cr; // used for responding back to the client after finishing a command

	if (command == "$register")
	{
		cr = CommandRegister(user, username); // do register logic
	}
	else if (command == "$exit")
	{
		cr = CommandExit(user); // give them the boot
	}
	else if (user.isRegistered)
	{
		if (command == "$getlist")
		{
			cr = CommandGetList(); // do get list logic
		}
		else if (command == "$getlog")
		{
			cr = USER_GET_LOG; // tells the return to do GetLog logic
		}
		else if (command == "$help")
		{
			cr = USER_HELP; // tells the return to do Help logic
		}
		else
		{
			cr = UKNOWN_COMMAND; // tell return they goofed and sent an incorrect command love letter
		}
	}
	else cr = NOT_REGISTERED; // they goofed and tried to send command love letters for commands they dont have access to yet

	return cr;
}
Server::ServerError Server::CommandResponse(User& user, CommandResult commandResult)
{
	ServerError result;
	// server processed the command, so echo back to the client
	switch (commandResult)
	{
	case SERVER_CAPACITY_FULL: // Registration Failed
	{
		result = EchoPreset(user, 2); // tell the client there is no room for it on the server
	} break;

	case USER_ACCEPTED: // Registration success
	{
		// Registration Success: Server sends "SV_SUCCESS", client send back "CL_RECEIVED", then send new user msg
		result = EchoPreset(user, 1); // tell client registration was a success "SV_SUCCESS"
		if (result != SUCCESS) return result;
		ServerError result = ReceiveMessage(user); // receive a love letter
		if (result != SUCCESS && result != DISCONNECT) return result; // no more love letters if there was an issue
		else if (result == DISCONNECT) ClientDisconnect(user); // client did an ungraceful disconnect so boot them
		else
		{
			// Client said it received the success message so let's announce the new user in chat
			string msg = "[Server]: ";
			msg.append(user.Username);
			msg.append(" has joined the chat room.");
			result = EchoToOthers(msg); // Echo the love letter "Username has joined the char room."
			if (result != SUCCESS) return result;
			printf("%s\n", msg.c_str()); // print the new user love letter to the server's chat
		}
	} break;

	case USER_GET_LIST:
	{
		result = Echo(user, userList); // Echo the love letter "list of connected and registered users"
		if (result != SUCCESS) return result;
	} break;

	case LIST_EMPTY: // $getlist list was empty
	{
		result = EchoPreset(user, 6); // tell the client the userlist is empty
	} break;
	case UKNOWN_COMMAND:
	{
		result = EchoPreset(user, 7); // tell the client the command they entered was not found
	}break;
	case USER_GET_LOG:
	{
		result = SendLog(user); // does send log logic
	}
	case USER_EXITED:
	{
		result = SUCCESS; // user successfully left
	} break;
	case USER_HELP:
	{
		result = EchoPreset(user, 4); // tell the client to print out the help information
	}break;
	case NOT_REGISTERED:
	{
		result = EchoPreset(user, 2); // tell the client there is no room for it on the server
	} break;
	case ALREADY_REGISTERED:
	{
		result = EchoPreset(user, 9); // tell the client that they have already registered with the server
	} break;
	case ECHO_ERROR:
	{
		return ECHO_ALL_FAIL; // there was an error went sending to each user :(
	} break;
	default:
	{
		result = COMMAND_RESPONSE_FAIL; // uh oh how did you let this get in here
	} break;
	}
	return result;
}
Server::CommandResult Server::CommandRegister(User& user, string username)
{
	if (!user.isRegistered) // we only want to do this logic if they arent registered
	{
		if (connection_capacity < max_capacity) // check if we have room for the client
		{
			// we have room woot woot
			connection_capacity++; // add one to the capacity
			user.Username = username; // set the username
			// add them to the userList
			userList.append(username.c_str());
			userList.append(",");
			RemoveUserFromVector(user); // remove the user from the other users list
			user.isRegistered = true; // set client to registered
			users_registered.push_back(user); // add the user to the registered user list
			return USER_ACCEPTED; // tell the server there was room for the client
		}
		else
		{
			user.isRegistered = false; // client is not allowed to send love letters
			return SERVER_CAPACITY_FULL; // tell server there was no room for the client
		}
	}
	else return ALREADY_REGISTERED; // tell server they are already registered
}
Server::CommandResult Server::CommandGetList()
{
	if (connection_capacity > 0) // check if there
	{
		return USER_GET_LIST; // tell server we are gonna send the list string love letter
	}
	else return LIST_EMPTY; // tell the server the list string love letter is empty
}
Server::CommandResult Server::CommandExit(User& user)
{
	ServerError result = EchoPreset(user, 10);
	result = ClientDisconnect(user);
	if (result != SUCCESS) return ECHO_ERROR;
	return USER_EXITED;
}

// Server love letter cl_
Server::ServerError Server::CL_Received(User& user, string buffer, int bufferSize)
{
	string CL_MESSAGE = CL_Data(buffer.c_str(), bufferSize); // break the love letter up so the server knows what comes after CL_

	MessageResult messageResult = CL_Process(CL_MESSAGE); // complete the logic for the message that was entered

	ServerError result = CL_Response(user, messageResult); // echo back to the client if needed else itll return success
	return result;
}
string Server::CL_Data(const char* buffer, int bufferSize)
{
	string message;
	bool isCL = true;
	// get the command they put in
	int i = 3;
	while (true)
	{
		message += buffer[i];
		++i;
		if (buffer[i] == '\0') break;
		if (i >= bufferSize) break;
	}
	return message;
}
Server::MessageResult Server::CL_Process(string msg)
{
	MessageResult mr; // used for responding back to the client after finishing the message logic
	if (msg == "READY") mr = USER_READY; // we got their response so the server is good to continue
	else mr = UNKOWN_MESSAGE; // they goofed and sent an incorrect CL_ love letter
	return mr;
}
Server::ServerError Server::CL_Response(User& user, MessageResult messageResult)
{
	ServerError result;
	switch (messageResult)
	{
	case UNKOWN_MESSAGE:
	{
		result = EchoPreset(user, 8); // tell the client the CL_ love letter they entered was not found
	} break;
	case USER_READY:
	{
		result = SUCCESS; // Server doesnt need to echo anything back to the client right now (will echo something after this returns success)
	} break;
	default:
	{
		result = CL_RESPONSE_FAIL; // uh oh how did you let this get in here
	}break;
	}
	return result;
}

// Server util
Server::ServerError Server::ClientDisconnect(User& user)
{
	// uh oh someone left
	// remove them from the active user list
	string remove_me = user.Username;
	remove_me.append(",");
	RemoveWordFromLine(userList, remove_me); // search the list and removes whatever the second param contains
	connection_capacity--; // minus 1 from the current capacity
	RemoveUserFromVector(user);
	// tell the chat room
	if (user.isRegistered)
	{
		string msg = "[Server]: ";
		msg.append(user.Username);
		msg.append(" has disconnected from the chat room.");
		ServerError result = EchoToOthers(user, msg); // tell every user, but this user, that this user is disconnected
		if (result != SUCCESS) return result;

		RemoveWordFromLine(msg, "[Server]: ");
		LogMessage("Server", msg);
		printf("%s\n", msg.c_str()); // print the love letter to the server's chat
	}
	else printf("New client attempted to join but server was full.");
	FD_CLR(user.CommunicationSocket, &master);
	return SUCCESS;
}
void Server::RemoveWordFromLine(string& line, const string& word)
{
	auto n = line.find(word);
	if (n != string::npos) line.erase(n, word.length());
}
void Server::ShutdownUser(User& user)
{
	// Shutdown and Close the User's Communication Socket
	shutdown(user.CommunicationSocket, SD_BOTH);
	closesocket(user.CommunicationSocket);
}
void Server::ShutdownServer()
{
	string msg = "[Server]: Server is shutting down. Goodbye!";
	ServerError r = EchoToOthers(msg); // tell every user in the users vector server is shutting down
	// sleep for 5 seconds to wait for the echo to others to send
	this_thread::sleep_for(chrono::seconds(5));
	for (int i = 0; i < users_registered.size(); ++i) EchoPreset(users_registered[i], 10); // tell each registered user to exit
	for (int i = 0; i < users_not_registered.size(); ++i) EchoPreset(users_not_registered[i], 10); // tell each unregistered user to exit
	// sleep for 5 seconds to wait for the echo presets to send
	this_thread::sleep_for(chrono::seconds(5));
	// shutdown and close each user communication socket
	FD_CLR(ListenSocket, &master);
	// shutdown and close the server's listen and broadcast sockets
	shutdown(ListenSocket, SD_BOTH);
	closesocket(ListenSocket);

	shutdown(BroadcastSocket, SD_BOTH);
	closesocket(BroadcastSocket);
}
void Server::DisplayError(ServerError errorNumber)
{

	switch (errorNumber)
	{
	case SUCCESS:
	{

	} break;
	case DISCONNECT:
	{
		printf("ERROR RESULT = DISCONNECT\n");
	} break;
	case SHUTDOWN:
	{
		printf("ERROR RESULT = SHUTDOWN\n");
	} break;
	case SOCKET_FAIL:
	{
		printf("ERROR RESULT = SOCKET FAIL\n");
	} break;
	case ADDRESS_FAIL:
	{
		printf("ERROR RESULT = ADDRESS FAIL\n");
	} break;
	case CONNECTION_FAIL:
	{
		printf("ERROR RESULT = CONNECTION FAIL\n");
	} break;
	case PARAM_FAIL:
	{
		printf("ERROR RESULT = PARAMETER FAIL\n");
	} break;
	case BIND_FAIL:
	{
		printf("ERROR RESULT = BIND FAIL\n");
	} break;
	case LISTEN_FAIL:
	{
		printf("ERROR RESULT = LISTEN FAIL\n");
	} break;
	case ACCEPT_FAIL:
	{
		printf("ERROR RESULT = ACCEPT FAIL\n");
	} break;
	case ECHO_ALL_FAIL:
	{
		printf("ERROR RESULT = ECHO TO ALL FAIL\n");
	} break;
	case COMMAND_RESPONSE_FAIL:
	{
		printf("ERROR RESULT = COMMAND RESPONSE FAIL\n");
	} break;
	case CL_RESPONSE_FAIL:
	{
		printf("ERROR RESULT = CL RESPONSE FAIL\n");
	} break;
	case SELECT_FAIL:
	{
		printf("ERROR RESULT = SELECT FAIL\n");
	} break;
	case FIND_USER_FAIL:
	{
		printf("ERROR RESULT = FAILED TO FIND WHICH USER BELONGED TO THE COM SOCKET\n");
	} break;
	case SERVER_SHUTDOWN:
	{
		// nothing
	}break;
	case UDP_SETUP_FAIL:
	{
		printf("ERROR RESULT = FAILED SETTING UP UDP SOCKET & OPTIONS\n");
	} break;
	case BROADCAST_FAIL:
	{
		printf("ERROR RESULT = FAILED BROADCAST OF SERVER INFORMATION\n");
	} break;
	default:
	{
		printf("ERROR RESULT = UNKNOWN ERROR\n");
	} break;
	}

}
bool Server::isWhiteSpace(unsigned char c)
{
	if (c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') return true;
	else return false;
}
void Server::RemoveUserFromVector(User user)
{
	bool isAtBegin = false;
	if (user.isRegistered)
	{
		// remove the user from the other users list
		bool isAtBegin = false;
		if (users_registered[0].CommunicationSocket != user.CommunicationSocket)
		{
			for (int i = 0; i < users_registered.size(); ++i) // check to see where in the users_not_registered list this user is at
			{
				if (users_registered[i].CommunicationSocket == user.CommunicationSocket) // we found our user
				{
					User temp = users_registered[0];
					users_registered[0] = user;
					users_registered[i] = temp;
					isAtBegin = true;
				}
			}
		}
		else isAtBegin = true;
		if (isAtBegin) users_registered.erase(users_registered.begin());
	}
	else
	{
		// remove the user from the other users list
		if (users_not_registered[0].CommunicationSocket != user.CommunicationSocket)
		{
			for (int i = 0; i < users_not_registered.size(); ++i) // check to see where in the users_not_registered list this user is at
			{
				if (users_not_registered[i].CommunicationSocket == user.CommunicationSocket) // we found our user
				{
					User temp = users_not_registered[0];
					users_not_registered[0] = user;
					users_not_registered[i] = temp;
					isAtBegin = true;
				}
			}
		}
		else isAtBegin = true;
		if (isAtBegin) users_not_registered.erase(users_not_registered.begin());
	}
}

// Log stuff
void Server::LogMessage(string name, string msg)
{
	log_line_count++; // we are adding a new line to the file so up the count
	string fileName = "ServerLog.txt";

	string logMe = "[";
	logMe.append(name);
	logMe.append("]: ");
	logMe.append(msg);

	fstream file;
	file.open(fileName, std::fstream::in | std::fstream::out | std::fstream::app);
	file << logMe << "\n";
	file.close();
}
void Server::CreateLogFile()
{
	// replaces or create the log.txt then adds "# Server Log File" as the first line
	string fileName = "ServerLog.txt";
	fstream new_file;
	new_file.open(fileName, std::fstream::in | std::fstream::out | std::fstream::trunc);
	new_file << "# Server Log File\n";
	new_file.close();
	log_line_count++;
}
Server::ServerError Server::SendLog(User user)
{
	//tell the user to get ready for the line count
	ServerError result = EchoPreset(user, 5); // tell the client to get ready for reading log data

	// send the client how many lines to read in
	string lines_sending = to_string(log_line_count);
	result = sendMessage(user, lines_sending, false);
	if (result != SUCCESS) return result;

	// now read each lines and send them it
	string line;
	fstream log_file;
	log_file.open("ServerLog.txt");
	while (getline(log_file, line)) // until we reach the end of the file
	{
		result = sendMessage(user, line, false); // send the current line
		if (result != SUCCESS) return result;
	}
	log_file.close();

	// tell the user we are done sending the lines
	result = EchoPreset(user, 11);
	if (result != SUCCESS) return result;

	return SUCCESS;
}