#ifndef __CLIENT_H
#define __CLIENT_H

#include <winsock2.h>
#include <string>

class Client
{
public:
	Client(std::string server, unsigned short int port);
	~Client();

	std::string send(std::string request);
	void OpenConnection();
	void CloseConnection();

private:
	std::string server;
	unsigned short int port;
	std::string portString;

	SOCKET ConnectSocket;
};

#endif // __CLIENT_H
