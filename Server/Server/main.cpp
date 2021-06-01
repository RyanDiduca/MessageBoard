#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include "Server.h"
#include "RequestParser.h"
#include "Client.h"

#define DEFAULT_PORT 12345

void threadFunction(Server server, ReceivedSocketData&& data);

std::mutex mutex;
std::unordered_map<std::string, std::unordered_map<int, std::string>> TopicMap;

bool terminateServer = false;

int main()
{
	Server server(DEFAULT_PORT);
	ReceivedSocketData receivedData;
	std::vector<std::thread>ThreadList;//Create a thread for each client
	
	std::cout << "Starting server. Send \"exit\" (without quotes) to terminate." << std::endl;

	while (!terminateServer)
	{
		receivedData = server.accept();
		if (!terminateServer)
		{
			ThreadList.emplace_back(std::thread(threadFunction, server, receivedData));
		}
	}

	for (auto &t : ThreadList)
	{
		t.join();
	}

	std::cout << "Server terminated." << std::endl;

	return 0;
}

void threadFunction(Server server, ReceivedSocketData&& data)
{
	PostRequest post;
	ReadRequest read;
	CountRequest count;
	ListRequest list;
	ExitRequest exit;
	std::unordered_map<int, std::string> tempMap;
	std::string topicList, readRequest;
	int topicCount, lastPostID = -1;
	bool counter = false, exitFlag = false;

	do {
		server.receiveData(data, 0);
		post = PostRequest::parse(data.request);
		if (post.valid)
		{
			mutex.lock();
			if (TopicMap.find(post.topicId.substr(0, 140)) != TopicMap.end())
			{
				TopicMap.find(post.topicId.substr(0, 140))->second.insert({ TopicMap.find(post.topicId.substr(0, 140))->second.size(), post.message.substr(0, 140) });
				lastPostID = TopicMap.find(post.topicId.substr(0, 140))->second.size() - 1;
			}
			else
			{
				tempMap.clear();
				tempMap.insert({ 0, post.message.substr(0, 140) });
				TopicMap.insert({ post.topicId.substr(0, 140), tempMap });
				lastPostID = 0;
			}
			mutex.unlock();
			data.reply = std::to_string(lastPostID);
			server.sendReply(data);
			continue;
		}
		read = ReadRequest::parse(data.request);
		if (read.valid)
		{
			readRequest = "";
			mutex.lock();
			if (TopicMap.find(read.topicId.substr(0, 140)) != TopicMap.end())
			{
				if (read.postId >= 0 && read.postId < TopicMap.find(read.topicId.substr(0, 140))->second.size())
				{
					readRequest = TopicMap.find(read.topicId.substr(0, 140))->second.at(read.postId);
				}
			}
			mutex.unlock();
			data.reply = readRequest;
			server.sendReply(data);
			continue;
		}
		count = CountRequest::parse(data.request);
		if (count.valid && data.request != "" && data.request != "exit")
		{
			topicCount = 0;
			mutex.lock();
			if (TopicMap.find(count.topicId.substr(0, 140)) != TopicMap.end())
			{
				topicCount = TopicMap.find(count.topicId.substr(0, 140))->second.size();
			}
			mutex.unlock();
			data.reply = std::to_string(topicCount);
			server.sendReply(data);
			continue;
		}
		list = ListRequest::parse(data.request);
		if (list.valid && data.request != "" && data.request != "exit")
		{
			topicList = "";
			mutex.lock();
			for (auto& x : TopicMap)
			{
				if (counter == true)
				{
					topicList.append("#");
				}
				topicList.append(x.first);
				counter = true;
			}
			mutex.unlock();
			counter = false;
			data.reply = topicList;
			server.sendReply(data);
			continue;
		}
		exit = ExitRequest::parse(data.request);
		if (exit.valid)
		{
			data.reply = "TERMINATING";
			exitFlag = true;
			server.sendReply(data);
			continue;
		}
		if (data.request != "" && data.request != "exit")
		{
			data.reply = "";
			server.sendReply(data);
		}
	} while (!terminateServer && !exitFlag);

	if (!terminateServer && data.request == "exit")
	{
		terminateServer = true;

		Client tempClient(std::string("127.0.0.1"), DEFAULT_PORT);
		tempClient.OpenConnection();
		tempClient.CloseConnection();
	}
	server.closeClientSocket(data);
}