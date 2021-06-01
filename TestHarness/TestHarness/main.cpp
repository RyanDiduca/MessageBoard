#include <iostream>
#include <thread>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unordered_map>
#include <mutex>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>

#define DEFAULT_PORT "12345"
#define PORT_BUFFER_LEN 10
#define DEFAULT_BUFLEN 65536

std::unordered_map<int, std::string> Strings;
std::unordered_map<int, double> PostCount;
std::unordered_map<int, double> ReadCount;
std::unordered_map<int, double> PostTimes;
std::unordered_map<int, double> ReadTimes;

void postFunction(SOCKET&& conSocket, double timer, int id);
void readFunction(SOCKET&& conSocket, double timer, int id);
SOCKET clientMaker(addrinfo* hints, addrinfo* result);
std::string generateString();

std::mutex mutex;

int main(int argc, char** argv)
{
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		exit(1);
	}

	addrinfo hints, * result = NULL;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result))
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	// Validate the parameters
	if (argc != 5) {
		printf("usage: %s server-name|IP-address   Number of Poster Threads   Number of Reader Threads   Execution Time(s)\n", argv[0]);
		return 1;
	}
	double timer = std::stoi(argv[4]);
	int posters = std::stoi(argv[2]);
	int readers = std::stoi(argv[3]);
	
	int totalPosts = 0, totalReads = 0;

	std::vector<SOCKET> sockets;
	std::vector<std::thread>PostList;
	std::vector<std::thread>ReadList;

	for (int i = 0; i < 1000; i++)
	{
		Strings.insert({i, generateString()});
	}
	for (int i = 0; i < std::stoi(argv[2]); i++)
	{
		SOCKET ConSocket = clientMaker(&hints, result);
		PostList.emplace_back(std::thread(postFunction, ConSocket, timer, i));
	}
	for (int i = 0; i < std::stoi(argv[3]); i++)
	{
		SOCKET ConSocket = clientMaker(&hints, result);
		ReadList.emplace_back(std::thread(readFunction, ConSocket, timer, i));
	}
	for (auto& p : PostList)
	{
		p.join();
	}
	for (auto& r : ReadList)
	{
		r.join();
	}
	SOCKET conSocket = clientMaker(&hints, result);
	send(conSocket, "EXIT", 5, 0);
	for (int i = 0; i < std::stoi(argv[2]); i++)
	{
		std::cout << "POST Thread " << i << " sent:\n";
		std::cout << "Average: " << (PostCount.find(i)->second / timer) << " requests\n";
		std::cout << "Runtime: " << PostTimes.find(i)->second << " seconds\n\n";
		totalPosts += PostCount.find(i)->second;
	}
	for (int i = 0; i < std::stoi(argv[3]); i++)
	{
		std::cout << "READ Thread " << i << " sent:\n";
		std::cout << "Average: " << (ReadCount.find(i)->second / timer) << " requests\n";
		std::cout << "Runtime: " << ReadTimes.find(i)->second << " seconds\n\n";
		totalReads += ReadCount.find(i)->second;
	}
	std::cout << "Total POSTS: " << totalPosts << std::endl;
	std::cout << "Total READS: " << totalReads << std::endl;
	std::cout << "Total Requests: " << (totalPosts + totalReads) << std::endl;
	std::cout << "Average Requests Per Thread Per Second: " << (((totalPosts + totalReads) / (posters + readers)) / timer) << std::endl;
	WSACleanup();
	system("pause");
	return 0;
}

std::string generateString()
{
	static const char characters[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	srand(time(NULL));
	std::string temp;
	int length = rand() % 150 + 4;
	for (int i = 0; i < length; i++)
	{
		temp += characters[rand() % (sizeof(characters) - 1)];
	}
	return temp;
}

SOCKET clientMaker(addrinfo* hints, addrinfo* result)
{
	addrinfo* ptr;
	SOCKET conSocket = INVALID_SOCKET;
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		conSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (conSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			exit(1);
		}
		if (connect(conSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
		{
			closesocket(conSocket);
			conSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	return conSocket;
}

void postFunction(SOCKET&& conSocket, double timer, int id)
{
	LARGE_INTEGER liStart, liEnd, elapsedMicroseconds;
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&liStart);
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN - 1;
	int count = 0;
	int res = -1;
	std::string sendstr;
	srand(time(NULL));
	do {
		sendstr = "POST@" + Strings.find(rand() % 1000)->second + "#" + Strings.find(rand() % 1000)->second;
		res = ::send(conSocket, sendstr.c_str(), (int)sendstr.size() + 1, 0);
		if (res == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(conSocket);
			WSACleanup();
			exit(1);
		}
		res = recv(conSocket, recvbuf, recvbuflen, 0);
		QueryPerformanceCounter(&liEnd);
		count++;
	} while ((double(liEnd.QuadPart - liStart.QuadPart) / Frequency.QuadPart) < timer);
	mutex.lock();
	PostTimes.insert({ id, (double(liEnd.QuadPart - liStart.QuadPart) / Frequency.QuadPart) });
	mutex.unlock();
	mutex.lock();
	PostCount.insert({ id, count });
	mutex.unlock();
}

void readFunction(SOCKET&& conSocket, double timer, int id)
{
	LARGE_INTEGER liStart, liEnd, elapsedMicroseconds;
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&liStart);
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN - 1;
	int count = 0;
	int result = -1;
	std::string sendstr;
	srand(time(NULL));
	do {
		sendstr = "READ@" + Strings.find(rand() % 1000)->second + "#" + std::to_string(rand() % 10);
		result = ::send(conSocket, sendstr.c_str(), (int)sendstr.size() + 1, 0);
		if (result == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(conSocket);
			WSACleanup();
			exit(1);
		}
		result = recv(conSocket, recvbuf, recvbuflen, 0);
		QueryPerformanceCounter(&liEnd);
		count++;
	} while ((double(liEnd.QuadPart - liStart.QuadPart) / Frequency.QuadPart) < timer);
	mutex.lock();
	ReadTimes.insert({ id, (double(liEnd.QuadPart - liStart.QuadPart) / Frequency.QuadPart) });
	mutex.unlock();
	mutex.lock();
	ReadCount.insert({ id, count });
	mutex.unlock();
}