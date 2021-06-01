#ifndef __SERVER_H
#define __SERVER_H

#include "ReceivedSocketData.h"

class Server
{
public:
	Server(unsigned short int port);
	~Server();
	ReceivedSocketData accept();
	void receiveData(ReceivedSocketData& ret, bool blocking);
	int sendReply(ReceivedSocketData reply);

	void OpenListenSocket();
	void CloseListenSocket();
	int closeClientSocket(ReceivedSocketData &reply);

private:
	SOCKET ListenSocket;
	unsigned short int port;
	std::string portString;
};

#endif __SERVER_H
