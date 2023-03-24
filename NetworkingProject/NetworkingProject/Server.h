#pragma once
#include "Definitions.h"

class Server
{
public:

	struct User
	{
		SOCKET CommunicationSocket;
		struct sockaddr_in address;
		string Username;
		bool isRegistered;
	};
	struct Command
	{
		string command;
		string input;
	};
	enum CommandResult
	{
		UKNOWN_COMMAND = 0, LIST_EMPTY = 1, SERVER_CAPACITY_FULL = 2, USER_ACCEPTED = 3, USER_GET_LIST = 4, USER_GET_LOG = 5, USER_EXITED = 6, USER_HELP = 7, NOT_REGISTERED = 8, ALREADY_REGISTERED = 9,
		ECHO_ERROR = 10
	};
	enum MessageResult
	{
		UNKOWN_MESSAGE = 0, USER_READY = 1
	};
	enum ServerError
	{
		SUCCESS = 0, DISCONNECT = 1, SHUTDOWN = 2,
		SOCKET_FAIL = 3, ADDRESS_FAIL = 4, CONNECTION_FAIL = 5,
		PARAM_FAIL = 6, BIND_FAIL = 9, LISTEN_FAIL = 10, ACCEPT_FAIL = 11,
		ECHO_ALL_FAIL = 12, COMMAND_RESPONSE_FAIL = 13, CL_RESPONSE_FAIL = 14,
		SELECT_FAIL = 15, FIND_USER_FAIL = 16, SERVER_SHUTDOWN = 17, UDP_SETUP_FAIL = 18,
		BROADCAST_FAIL = 19
	};
	// Server love letter setup
	ServerError CreateListenTcpSocket();
	ServerError CreateUDPSocketBroadcast();
	ServerError AcceptIncomingConnections(Server& server_instance);

	// public receiving love letters functions
	void LoopReceiving(User& user);

	// public util functions
	void ShutdownServer();
	void DisplayError(ServerError errorNumber);

private:
	
	SOCKET ListenSocket;
	SOCKET BroadcastSocket;
	sockaddr_in broadcast_address;
	sockaddr_in server_address;
	uint16_t port;
	string address;
	uint16_t broadcast_port = 31338;

	unsigned int max_capacity = 4;
	unsigned int connection_capacity = 0;
	vector<User> users_registered;
	vector<User> users_not_registered;
	string userList = "[Server]: ";

	fd_set master; // no calling select with
	fd_set ready; // yes calling select with

	unsigned log_line_count = 0;

	//private server sending love letter functions
	int tcp_send_whole(SOCKET skSocket, const char* data, uint16_t length);
	ServerError sendMessage(SOCKET& ComSocket, char* data, int32_t length);
	ServerError sendMessage(User& user, string message, bool logMessage);
	ServerError EchoToOthers(User user, string buffer);
	ServerError EchoToOthers(string buffer);
	ServerError EchoPreset(User& user, int preset);
	ServerError Echo(User& user, string msg);
	ServerError EchoChatMessage(User& user, string msg);

	// private Server receiving love letter functions
	ServerError BroadcastInformation();
	int tcp_recv_whole(SOCKET s, char* buf, int len);
	ServerError readMessage(SOCKET& ComSocket, char* buffer, int32_t size, unsigned& read_size);
	ServerError ReceiveMessage(User& user);
	ServerError ProcessMessage(User& user, string msg);

	// private server command handling functions
	ServerError CommandReceived(User& user, string buffer, int bufferSize);
	Command CommandData(const char* buffer, int bufferSize);
	CommandResult CommandProcess(string command, User& user, string username);
	ServerError CommandResponse(User& user, CommandResult commandResult);
	// command functions
	CommandResult CommandRegister(User& user, string username);
	CommandResult CommandGetList();
	CommandResult CommandExit(User& user);

	// private server handling CL_MESSAGES functions
	ServerError CL_Received(User& user, string buffer, int bufferSize);
	string CL_Data(const char* buffer, int bufferSize);
	MessageResult CL_Process(string msg);
	ServerError CL_Response(User& user, MessageResult messageResult);

	// private Server util functions
	ServerError ClientDisconnect(User& user);
	void RemoveWordFromLine(string& line, const string& word);
	void ShutdownUser(User& user);
	bool isWhiteSpace(unsigned char c);
	void RemoveUserFromVector(User user);

	// private log functions
	void LogMessage(string name, string msg);
	void CreateLogFile();
	ServerError SendLog(User user);

};

