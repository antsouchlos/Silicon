#include <unistd.h>
#include <thread>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#include "silicon.h"
#include "exceptions.h"

void isValidHex(char str[4]) {
	string valid = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	for (int i = 0; i < 4; i++)
		for (int i2 = 0; i2 < 36; i++)
			if (str[i] != valid.at(i2))
				throw SiliconException("Invalid prefix recieved: " + string(str));
}

//TODO solve mutex problem: the mutexes ar inside the client objects (cli->mtx). If the client object has been destroyed (from another thread) the mutex will also be destroyed
void client_recvListener(client *cli, int id, void (*client_recv)(int id, string message)) {
	bool running = true;

	while (running) {
		cli->mtx.lock();
			int socket = cli->socket;
		cli->mtx.unlock();

		char prefix[4];
		char buffer[256];
		int n = 0;

		//read prefix (number of bytes to be recieved in hexadecimal)
		while (n < 4) {
			if ((n = read(socket, prefix, 4 - n)) < 0)
				throw SiliconException("Client (id:" + to_string(id) + "): Failed to read prefix from socket");
		}

		//read data
		isValidHex(prefix);
		int n_bytes = stoi(prefix, nullptr, 16);
		n = 0;
		while(n < n_bytes) {
			if ((n = read(socket, buffer, 255)) < 0)
				throw SiliconException("Client (id:" + to_string(id) + "): Failed to read data from socket");
			else if (n > 0)
				(*client_recv)(id, buffer);

			cout << "[DEBUG] n: " << n << endl;

			cli->mtx.lock();
			if (!cli->open) {
				close(cli->socket);
				running = false;
				break;
			}
			cli->mtx.unlock();

		}

		usleep(100000);
	}
}

int Silicon::client_connect(string host, int port) {
	if (client_recv == nullptr)
		throw SiliconException("Client: no recieve-function has been set");

	int id = clients.size();

	struct client *cli = new client;

	cli->port = port;
	cli->open = true;

	//create a TCP IPv4 socket
	cli->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (cli->socket < 0)
		throw SiliconException("Client (id:" + to_string(id) + "): Failed to create socket");

	//put the socket in non-blocking mode (for the recv-funciton to work correctly)
	if (fcntl(cli->socket, F_SETFL, fcntl(cli->socket, F_GETFL) | O_NONBLOCK) < 0)
		throw SiliconException("Client: Failed to put the socket in non-blocking mode");

	//TODO examine if 'gethostbyname()' only works for domains or also for IPs. Note: try 'gethostbyip()'
	cli->server = gethostbyname(host.c_str());
	if (cli->server == NULL)
		throw SiliconException("Client (id:" + to_string(id) + "): No such host: '" + host + "'");

	//empty server_addr
	bzero((char *) &cli->server_addr, sizeof(cli->server_addr));

	//use IPv4
	cli->server_addr.sin_family = AF_INET;

	//copy the IP of the server into server_addr
	bcopy((char *)cli->server->h_addr, (char *)&cli->server_addr.sin_addr.s_addr, cli->server->h_length);

	//set the port (htons is used to convert from host- to network-byte order)
	cli->server_addr.sin_port = htons(port);

	if (connect(cli->socket, (struct sockaddr *) &cli->server_addr, sizeof(cli->server_addr)) < 0)
		throw SiliconException("Client (id:" + to_string(id) + "): An error occurred while trying to connect to host: '" + host + "'");

	cout << "[DEBUG] got here" << endl;

	clients.push_back(cli);

	thread t(client_recvListener, cli, client_indexOf(cli), client_recv);

	return id;
}

void Silicon::client_setRecv(void (*recv)(int id, string message)) {
	client_recv = recv;
}

//TODO multithreading error handling
void Silicon::client_send(int id, string msg) {
	client *cli = clients[id];

	if (write(cli->socket, msg.c_str(), msg.size()) < 0)
		throw SiliconException("client_send(): Failed to write data to socket");
}

void closeClient(client *cli) {
	cli->mtx.lock();
		cli->open = false;
	cli->mtx.unlock();
}

void Silicon::client_close(int id) {
	if (id >= clients.size())
		throw SiliconException("Client: No client with id '" + to_string(id) + "'");

	client *cli = clients[id];

	closeClient(cli);

	//client_close() and close() run on different threads, which means they can both delete clients/erase them from the vector without fear of a segfault (after calling closeClient(), which makes sure that client_recv() has stopped, which does run on a different thread)
	delete cli;
}

conClient *Silicon::client_getClientByID(int id) {
	if (id >= client_conClients.size())
		throw SiliconException("client_getClientByID(): No client with id '" + to_string(id) + "'");

	return client_conClients[id];
}

void Silicon::server_setRecv(void(*recv)(int server_id, int client_id, string message)) {
	server_recv = recv;
}

void acceptConnections(server *srv) {
	struct sockaddr_in *client_addr;
	socklen_t clientlen = sizeof(client_addr);

	int client_socket = accept(srv->server_socket, (struct sockaddr *) &client_addr, &clientlen);
	if (client_socket < 0)
		throw SiliconException("Server: Error while accepting client");

	//TODO handle accepted client (push back into srv->clients, etc.)

}

//TODO note: make it impossible for two servers to run on the same port (maybe?)
int Silicon::server(int port, void(*accept)(int server_id, int client_id), int n_clients) {
	for (int i : usedPorts)
		if (i == port)
			throw SiliconException("Server: port already in use");

	socklen_t clientlen;

	struct server *srv = new struct server;

	srv->server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (srv->server_socket < 0)
		throw SiliconException("Server: failed to create server-socket");

	bzero((char *) &srv->server_addr, sizeof(srv->server_addr));

	srv->port = port;
	srv->server_addr.sin_family = AF_INET;
	srv->server_addr.sin_addr.s_addr = INADDR_ANY;
	srv->server_addr.sin_port = htons(port);

	if (bind(srv->server_socket, (struct sockaddr *)&srv->server_addr, sizeof(srv->server_addr)) < 0)
		throw SiliconException("Server: Failed to bind");

	usedPorts.push_back(port);

	listen(srv->server_socket, n_clients);

	thread t(acceptConnections, srv);

	servers.push_back(srv);
}

//TODO
void Silicon::server_send(int serverid, int client_id, string msg) {

}

//TODO
void Silicon::server_sendToAll(int server_id, string message) {

}

//TODO
void Silicon::server_disconnect(int server_id, int client_id) {

}

//TODO
void Silicon::server_close(int server_id) {

}

conServer *Silicon::server_getServerByID(int server_id) {
	if (server_id >= server_conServers.size())
		throw SiliconException("server_getServerByID(): No server with id '" + to_string(server_id) + "'");

	return server_conServers[server_id];

}

conClient *Silicon::server_getClientByID(int server_id, int client_id) {
	if (server_id >= servers.size())
		throw SiliconException("server_getClientByID(): No server with id '" + to_string(server_id) + "'");

	struct server *srv = servers[server_id];

	if (client_id >= srv->clients.size())
		throw SiliconException("server_getClientByID(): No client with id '" + to_string(client_id) + "' in server (id:'" + to_string(server_id) + "')");


	return srv->clients[client_id];
}

//TODO also close servers
void Silicon::close() {
	vector<thread> threads;

	for (client *cli : clients) {
		thread t(closeClient, cli);
		threads.push_back(move(t));
	}

	for (int i = 0; i < threads.size(); i++)
		threads[i].join();

	for (client *cli : clients)
		delete cli;

	//clear the vector 'clients' and force and reallocate memory
	vector<struct client *>().swap(clients);
}

int Silicon::client_indexOf(client *client) {
	for (int i = 0; i < clients.size(); i++) {
		if (clients[i] == client)
			return i;
	}

	throw SiliconException("client_inxexOf(): No such client");
}
