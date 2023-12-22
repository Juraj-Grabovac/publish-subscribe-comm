/**
 ***********************************************************************
 * @file   client.cpp 
 * @author Juraj Grabovac (jgrabovac2@gmail.com)
 * @date   21/12/2023
 * @brief  This file is implementation of Client Application
 ***********************************************************************
*/


/*----- Includes -----*/
#include <iostream>
#include <sstream>
#include <WS2tcpip.h>


/*----- Defines -----*/
// Key event enter
#define ENDL 13
// Key event backspace
#define BS 8

using namespace std;



int main()
{
    // Initialize WinSock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		cerr << "Can't start Winsock, Err #" << wsResult << endl;
		return 0;
	}

	// Create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		cerr << "Can't create socket, Err #" << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}
        
    // Fill hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
    hint.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    string command, client_name, userInput;
    int connResult = SOCKET_ERROR;
    int recv_port;

    // Connection to server
    do
    {
        cout << ">>> ";
        
        getline(cin, userInput);
        istringstream iss(userInput);

        // Parse input stream to command, port and client name
        iss >> command >> recv_port >> client_name;
        
        // Check command
        if(command.compare("CONNECT") != 0)
        {
            cout << "Invalid command" << endl;
        }
        else if(client_name.empty())
        {
            cout << "Client name empty" << endl;
        }
        else
        {
            // Set port
            hint.sin_port = htons(recv_port);
            
            // Connect to server
            connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
            if (connResult == SOCKET_ERROR)
            {
                cerr << "Can't connect to server, try another port" << endl;
            }
        }
    } while(connResult != 0);
    
    
    // Set non-blocking(mode 1) client socket
    // Enabled send/recv without blocking while waiting for input 
    int mode = 1;
    ioctlsocket(sock, FIONBIO, (unsigned long *)&mode);
    
    // Create file descriptors for read and write
    fd_set read_flags, write_flags;
    
    // Wait time for an event
    struct timeval waitd = {10, 0};
  
    long unsigned int br;
    int sel, numRead;
    int endl_flag = 0;   

    char in[255];
    memset(&in, 0, 255);

    // Definition of std input handler and record
    HANDLE hIn;
    INPUT_RECORD input;
    
    // Clear user input
    userInput.clear();

    while(1) 
    {
        FD_ZERO(&read_flags);
        FD_ZERO(&write_flags);
        FD_SET(sock, &read_flags);
        FD_SET(sock, &write_flags);

        sel = select(sock+1, &read_flags, &write_flags, nullptr, &waitd);
        
        // Check select error
        if(sel < 0)
        {
            cerr << "Last Select Error, Err #" << WSAGetLastError() << endl;
            continue;
        }
        
        // If socket ready for reading
        if(FD_ISSET(sock, &read_flags)) 
        {
            // Clear read set
            FD_CLR(sock, &read_flags);

            // Read message from server
            memset(&in, 0, 255);
            numRead = recv(sock, in, 255, 0);
            
            if(numRead <= 0) 
            {
                printf("\nClosing socket");
                closesocket(sock);
                break;
            }
            else if(in[0] != '\0')
            {
                cout << in;
            }
        }

        /* ----- Receiving User Input ----- */
        // Get std input handler and check is there any input
        hIn = GetStdHandle(STD_INPUT_HANDLE);
        GetNumberOfConsoleInputEvents(hIn, &br);
        
        if(br)
        {
            // If there is console input, read it
            ReadConsoleInput(hIn, &input, 1, &br);
            
            // Check that input is a key event
            if(input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown == true)
            {
                // Discard input buffer and reset br - input already read and saved to string input
                FlushConsoleInputBuffer(hIn);
                br = 0;

                // Print input to console to keep read input visible
                cout << (char)input.Event.KeyEvent.wVirtualKeyCode;

                // Create user input and save to string userInput
                if ((input.Event.KeyEvent.wVirtualKeyCode != ENDL) && (input.Event.KeyEvent.wVirtualKeyCode != BS))
                {
                    userInput += (char)input.Event.KeyEvent.wVirtualKeyCode;
                }
                else if (input.Event.KeyEvent.wVirtualKeyCode == BS)
                {
                    if (userInput.empty() == 0)
                    {
                        userInput.pop_back();
                    }
                }
                else
                {
                    cout << endl;
                    endl_flag = 1;
                }
            }
        }
 
        // If socket ready for writing
        if(FD_ISSET(sock, &write_flags) && endl_flag) 
        {
            // Clear write set
            FD_CLR(sock, &write_flags);
            
            // Send message to server
            send(sock, userInput.c_str(), 255, 0);
            userInput.clear();
            
            // Reset end line flag to enable receiving new user input
            endl_flag = 0;
        }
    }

	// Close socket
	closesocket(sock);
    
    // Cleanup winsock
	WSACleanup();
    
    return 0;
}