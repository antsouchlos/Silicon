#include <iostream>

#include "silicon.h"

using namespace std;

void client_recv(int id, string message) {

}

int main() {
	Silicon s;
	s.client_setRecv(client_recv);

	int client = s.client_connect("192.168.1.13", 17390);
	s.client_send(1, "I am pi@oxylab");

	s.close();

//	int i = stoi("FA", nullptr, 16);
//	cout << i << endl;

	return 0;
}
