/**
 ***********************************************************************
 * @file   server.cpp 
 * @author Juraj Grabovac (jgrabovac2@gmail.com)
 * @date   21/12/2023
 * @brief  This file is implementation of Server Application
 ***********************************************************************
*/


/*----- Includes -----*/
#include <iostream>
#include <sstream>
#include <WS2tcpip.h>

using namespace std;


/*----- Defines -----*/
#define MAX_CLIENT_NUM 5


/*----- Enums and Structures -----*/
enum msg_type
{
    COMMAND,
    TOPIC,
    DATA
};

enum msg_command
{
    PUBLISH,
    SUBSCRIBE,
    UNSUBSCRIBE,
    INVALID_COMMAND
};

struct str_client
{
    SOCKET sock_handler;
    string subscribe_topic;
};

struct input_message
{
    string commandInput;
    string topicInput;
    string dataInput;
};

struct str_client client_subscribe_list[MAX_CLIENT_NUM];
struct input_message received_input_message;


/*----- Functions Prototypes -----*/
void initialize_client_subscribe_list(void);
void add_client_to_subscribe_list(SOCKET client);
void subscribe(SOCKET sock, string topic);
void unsubscribe(SOCKET sock, string topic);
int is_client_subscribed_to_publish_topic(SOCKET sock, string topic);
void remove_client_from_subscribe_list(SOCKET client);
void parse_input_message(string inputMessage);
msg_command resolveCommand(string input);



int main(int argc, char **argv)
{       
    int i = 0, j = 0, arg_size = 0;
    int port_num = 0, temp_port_num = 0;
    
    // Check input arguments
    if (argc != 2)
    {
        // Default port
        port_num = 54000;
    }
    else
    {
        // Calculate port - convert input argument to port
        while(argv[1][i] != '\0')
        {
            ++i;
        }
    
        arg_size = i; 
    
        while(i > 0)
        {
            j = i;
        
            --i;
            temp_port_num = argv[1][i] - '0';
        
            while(j < arg_size)
            {
                temp_port_num *= 10;
                ++j;
            }
        
            port_num += temp_port_num;
        } 
    } 
    
    cout << "Port: " << port_num << endl;
    
	// Initialze winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		cerr << "Can't Initialize winsock!" << endl;
		return 0;
	}
	
	// Create a listening socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		cerr << "Can't create a socket" << endl;
		return 0;
	}

	// Bind the ip address and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port_num);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; 
	
	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// Set socket for listening 
	listen(listening, SOMAXCONN);

	// Create file descriptor and zero it
	fd_set master;
	FD_ZERO(&master);

	// Add listening socket to file descriptor
	FD_SET(listening, &master);
    
    // Initialize client subscribe list
    initialize_client_subscribe_list();

	while (1)
	{
		// Make a copy of descriptor file because select() call is destructive
		fd_set copy = master;
        
        // Call select() 
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		// Loop through all the current connections
		for (int i = 0; i < socketCount; i++)
		{
			SOCKET sock = copy.fd_array[i];

			// Check is it a listening or a client socket
			if (sock == listening)
			{
				// Accept new connection
				SOCKET client = accept(listening, nullptr, nullptr);

				// Add the new connection to the list of connected clients
				FD_SET(client, &master);
                
                // Add the new client to the subscribe list
                add_client_to_subscribe_list(client);

				// Send a message to the connected client
				string ConnectMsg = "CLIENT CONNECTED\n";
				send(client, ConnectMsg.c_str(), ConnectMsg.size() + 1, 0);
			}
			else
			{
				char bufInput[4096];
				ZeroMemory(bufInput, 4096);
				
				// Receive message
				int bytesIn = recv(sock, bufInput, 4096, 0);
                
				if (bytesIn <= 0)
				{
					// Close client
					closesocket(sock);
					FD_CLR(sock, &master);
				}
                else if(strcmp(bufInput, "DISCONNECT") == 0)
                {
                    // Send a message to the disconnected client
                    string DisconnectMsg = "CLIENT DISCONNECTED\n";
                    send(sock, DisconnectMsg.c_str(), DisconnectMsg.size() + 1, 0);
                    
                    // Remove client from subscribe list
                    remove_client_from_subscribe_list(sock);
            
                    // Disconnect the client
                    closesocket(sock);
                    FD_CLR(sock, &master);
                }
				else
				{
                    string buf = bufInput;
                    string publish_topic;
                    int publish_flag = 0;
                    
                    // Parse input message before checking commands
                    parse_input_message(buf);
                    
                    // Convert string to enum so that switch-case could be performed 
                    switch(resolveCommand(received_input_message.commandInput))
                    {
                        case PUBLISH:
                        {
                            cout << "Publish command received" << endl;
                            
                            // Set publish flag and add publish topic 
                            publish_flag = 1;
                            publish_topic = received_input_message.topicInput;
                            
                            break;
                        }
                        case SUBSCRIBE:
                        {
                            cout << "Subscribe command received" << endl;
                            
                            // Subscribe client to specific topic 
                            subscribe(sock, received_input_message.topicInput);
                            
                            break;
                        }
                        case UNSUBSCRIBE:
                        {
                            cout << "Unsubscribe command received" << endl;
                            
                            // Unsubscribe client from specific topic
                            unsubscribe(sock, received_input_message.topicInput);
                            
                            break;
                        }
                        default:
                        {
                            cout << "Unknown command" << endl;
                        }
                    }

					// Check if there are messages waiting to be published to a specific topic and socket
					for (int i = 0; i < (int)master.fd_count; i++)
					{
						SOCKET outSock = master.fd_array[i];

                        // Check publish flag status and whether any client subscribed to publish topic
                        if (is_client_subscribed_to_publish_topic(outSock, publish_topic) && publish_flag)
						{
							ostringstream ss;
                            ss << "[Message] Topic: " << publish_topic << " Data: " << received_input_message.dataInput << endl;
                            
							string strOut = ss.str();
							send(outSock, strOut.c_str(), strOut.size() + 1, 0);
                            
                            // Clear publish flag and topic
                            publish_flag = 0;
                            publish_topic.clear();
						}
					}
				}
			}
		}
	}

	while (master.fd_count > 0)
	{
		// Get the socket
		SOCKET sock = master.fd_array[0];

		// Remove socket from the master file list and close the socket
		FD_CLR(sock, &master);
		closesocket(sock);
	}

	// Cleanup winsock
	WSACleanup();

    return 0;
}

// Initialization of the client subscribe list to default values 
void initialize_client_subscribe_list(void)
{
    int i;
    
    for(i = 0; i < MAX_CLIENT_NUM; ++i)
    {
        client_subscribe_list[i].sock_handler = INVALID_SOCKET;
        client_subscribe_list[i].subscribe_topic = "NO_TOPIC";
    }
    
    cout << "Init Done" << endl;
}

// Function add clients to subscribe list
void add_client_to_subscribe_list(SOCKET client)
{
    int i;
    
    for(i = 0; i < MAX_CLIENT_NUM; ++i)
    {
        if(client_subscribe_list[i].sock_handler == INVALID_SOCKET)
        {
            client_subscribe_list[i].sock_handler = client;
            cout << "Client added to subscribe list" << endl;
            break;
        }
    }
    
    if (i == MAX_CLIENT_NUM)
    {
        cout << "Client subscribe list full" << endl;
    }
}

// Function remove clients from subscribe list
void remove_client_from_subscribe_list(SOCKET client)
{
    int i;
    
    for(i = 0; i < MAX_CLIENT_NUM; ++i)
    {
        if (client_subscribe_list[i].sock_handler == client)
        {
            client_subscribe_list[i].sock_handler = INVALID_SOCKET;
            client_subscribe_list[i].subscribe_topic = "NO_TOPIC";
            cout << "Client removed from subscribe list" << endl;
            
            break;
        }
    }
}

// Function for parsing input message
void parse_input_message(string buf)
{
    size_t pos = 0;
    int delimeterCount = 0;
    string delimiter = " ";
    string commandPart;
    
    cout << "Parse input message" << endl;
    
    while (((pos = buf.find(delimiter)) != string::npos) || (buf.empty() == 0)) 
    {
        if(pos != string::npos)
        {
            commandPart = buf.substr(0, pos);
        }
        else
        {
            commandPart = buf;
        }
        
        switch(delimeterCount)
        {
            case COMMAND:
            {
                received_input_message.commandInput = commandPart;
                
                break;
            }
            case TOPIC:
            {
                received_input_message.topicInput = commandPart;
                
                break;
            }
            case DATA:
            {
                received_input_message.dataInput = commandPart;
                
                break;
            }
            default:
            {
                cout << "Invalid part of message" << endl;
            }
        }
        
        cout << "Client sent: " << commandPart << endl;
        
        if(pos != string::npos)
        {
            buf.erase(0, pos + delimiter.length());
        }
        else
        {
            buf.clear();
        }

        delimeterCount++;
    }
    
}

// Function for subscribing client to specific topic
void subscribe(SOCKET sock, string topic)
{
    int i;
    
    for(i = 0; i < MAX_CLIENT_NUM; ++i)
    {
        if(client_subscribe_list[i].sock_handler == sock)
        {
            client_subscribe_list[i].subscribe_topic = topic;
            cout << "Topic Subscribed" << endl;
            
            break;
        }
    }
}

// Function for unsubscribing client from specific topic
void unsubscribe(SOCKET sock, string topic)
{
    int i;
    
    for(i = 0; i < MAX_CLIENT_NUM; ++i)
    {
        if((client_subscribe_list[i].sock_handler == sock) && (client_subscribe_list[i].subscribe_topic == topic))
        {
            client_subscribe_list[i].subscribe_topic = "NO_TOPIC";
            cout << "Topic Unsubscribed" << endl;
            
            break;
        }
    }
}

// Function for checking is the client subscribed to specific topic
int is_client_subscribed_to_publish_topic(SOCKET sock, string topic)
{
    
    int retval = 0;
    int i;
    
    for(i = 0; i < MAX_CLIENT_NUM; ++i)
    {
        if((client_subscribe_list[i].sock_handler == sock) && (client_subscribe_list[i].subscribe_topic.compare(topic) == 0))
        {
            retval = 1;
            cout << "Subscribe topic found" << endl;
        }
    }
    
    return retval;
}

// Funtion converts input string to enum
msg_command resolveCommand(string input)
{
    if(input.compare("PUBLISH") == 0) return PUBLISH;
    if(input.compare("SUBSCRIBE") == 0) return SUBSCRIBE;
    if(input.compare("UNSUBSCRIBE") == 0) return UNSUBSCRIBE;
    
    return INVALID_COMMAND;
}