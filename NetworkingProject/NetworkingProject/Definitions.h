#pragma once
//includes
#include <iostream>
#include <winsock2.h>
#include <inttypes.h>
#include <WS2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <cstdlib>
#include <conio.h>
#include <stdio.h>
#include <chrono>
#include <fstream>
//defines
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS // turns of deprecated warnings
//using
using namespace std;

#ifndef _DEFINITIONS_H_
#define _DEFINITIONS_H_

// echoed server messages
const string SV_RECEIVED = "SV_RECEIVED", SV_SUCCESS = "SV_SUCCESS", SV_FULL = "SV_FULL", SV_REGISTER = "SV_REGISTER", SV_HELP = "SV_HELP", SV_LOG = "SV_LOG", SV_EMPTY = "SV_EMPTY", SV_NOT_COMMAND = "SV_NOT_COMMAND",
			SV_NOT_MESSAGE = "SV_NOT_MESSAGE", SV_ALR_REGISTERED = "SV_ALR_REGISTERED", SV_EXIT = "SV_EXIT", SV_LOG_END = "SV_LOG_END";

// echoed client messages
const string CL_READY = "CL_READY";

const string COMMANDS = "Client Commands\n[Server]: 1) $register \"Chat Name\" \t\t-> Register with the server.\n[Server]: 2) $exit \t\t\t-> Leave the server gracefully.\n[Server]: 3) $getlist \t\t\t-> Get the list of all registered users in the chat.\n[Server]: 4) $getlog \t\t\t-> Get a text file containing all commands and messages from each user.";

const unsigned BUFFER_SIZE = 1024;

#endif