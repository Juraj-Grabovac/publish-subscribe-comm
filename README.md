# About Implementation
This is the implementation of publish-subscribe communication using the TCP/IP protocol, that is, the server-client model.
The server is implemented so that it can handle multiple connection with clients. The message handling rules are implemented as requested in the task.

# Built With
- Source files are: server.cpp and client.cpp
- The programming language C++ and the c++17 standard were used
- The MinGW compiler was used for compiling
- Windows libraries are used, so before building it is necessary to set -lws2_32 to the linker option in order to include ws2_32 library
- Apart from the windows library, only standard libraries were used
- CodeLite IDE was used to write the code
- Execution of the application was started from the command prompt(cmd), and the command prompt interface was used as the user interface

# Getting Started
The server is started first and then the clients. 
Port can be assigned to the server during application startup. The port is sent as the first argument in main() function (run example: server.exe 1999). If no port is sent as an argument, then default port is assigned to server. 
After the server is started, the clients can be started. After starting the client application, it is necessary to enter the command CONNECT, and then the port and name of the client (example: CONNECT 1999 Client1). 
If connection between the server and the client was successful, the server prints CLIENT CONNECTED on the client interface.
After successful connection, all other commands(PUBLISH/SUBSCRIBE/UNSUBSCRIBE) can be sent in the way defined in the task. Space is used as a delimiter in the message, and the end of the message entry is marked with an enter.
During communication, logs are printed on the server and client interfaces. Most of the logs can be seen on the server interface, these logs are printed by the server during message handling and they are useful to see how the message handling process looks like.
The DISCONNECT command disconnects the client from the server.
