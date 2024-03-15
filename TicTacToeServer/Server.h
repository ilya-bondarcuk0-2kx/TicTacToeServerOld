#pragma once


#define _WINSOCK_DEPRECATED_NO_WARNINGS



#include<WinSock2.h>
#include <ws2tcpip.h>	
#include<thread>
#include<iostream>
#include<nlohmann/json.hpp>
#include<atomic>
#include<vector>
#include<mutex>
#include"Client.h"

class Server
{
public:

	using json = nlohmann::json;

	Server(const char* host, int port);
	

	
	void run();


private: //server functions for talking with client

	int init();
	int send_to_client(SOCKET& socket, std::string buffer);
	int send_to_client(SOCKET& socket, json& json);
	void broadcast_to_clients(std::string buffer);
	void broadcast_to_clients(SOCKET& socket, std::string buffer);
	int receive_from_client(SOCKET& socket, std::string& buffer);
	
private: // other methods

	void admin_panel();
	void connections_accept();
	void refresh_clients();

private: // for winsock2 lib

	bool init_ws();
	void close_ws();

private:

	SOCKET listening_socket;
	const char* host;
	int port;
	std::atomic_int conn_count;
	int turn;
	std::atomic_bool active;
	sockaddr_in hint;
	std::string current_side;
	std::vector<SOCKET> connections;
	std::vector<Client> clients;
	
	


};

