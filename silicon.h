#ifndef SILICON_H
#define SILICON_H

#include <iostream>
#include <vector>
#include <netinet/in.h>
#include <netdb.h>
#include <mutex>

using namespace std;

struct client {
	bool open;
	mutex mtx;

	int port;
	int socket;
	struct hostent *server;
	struct sockaddr_in server_addr;
};

struct server {
	int port;
	int server_socket;
	struct sockaddr_in server_addr;
	vector<struct sockaddr_in> cli_addrs;
	vector<int> client_sockets;
	vector<struct conClient *> clients;
};

struct conClient {
	int port;
	string ip;
};

struct conServer {
	int port;
	int n_clients = 0;
};

class Silicon {
public:
	//---- CLIENT FUNCTIONS ----

	int client_connect(string ip, int port);
	void client_setRecv(void(*recv)(int id, string message));
	void client_send(int id, string msg);
	void client_close(int id);
	conClient *client_getClientByID(int id);

	//---- SERVER FUNCTIONS ----

	void server_setRecv(void(*recv)(int server_id, int client_id, string message));
	//TODO
	int server(int port, void(*accept)(int server_id, int client_id), int n_clients=5);
	//TODO
	void server_send(int server_id, int client_id, string msg);
	//TODO
	void server_sendToAll(int server_id, string message);
	//TODO
	void server_disconnect(int server_id, int client_id);
	//TODO
	void server_close(int server_id);
	conServer *server_getServerByID(int server_id);
	conClient *server_getClientByID(int server_id, int client_id);

	//---- GENERAL FUNCTIONS ----

	//TODO
	void close();
private:
	vector<struct client *> clients;
	vector<struct server *> servers;
	vector<struct conClient *> client_conClients;
	vector<struct conServer *> server_conServers;
	void (*client_recv)(int client_id, string message) = nullptr;
	void (*server_recv)(int server_id, int client_id, string message) = nullptr;

	vector<int> usedPorts;

	int client_indexOf(client *client);
};

#endif
