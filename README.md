# MessageBoard
A multithreaded application to handle traffic to and from a message board.
A central server application written in c++ for a simple discussion forum geared towards high traffic and short messages. Multiple clients can connect concurrently over a network and post & read messages on multiple topics.
The server communicates with clients over TCP sockets, listening for connections on port 12345.
The project also contains a test harness to simulate a user defined number of poster and reader threads firing a large number of requests to the server continuously over a set time period. 

Once the server.exe is running, the test harness can be run via the command prompt by navigating to the x64 release folder and entering: "TestHarness.exe 127.0.0.1 2 3 5".
In the above example a harness will be created with 2 poster threads and 3 reader threads over a period of 5 seconds, however different variables can be entered.
