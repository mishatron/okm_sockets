
#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <thread>
#include <sys/types.h> 
#include<iostream>
#include<vector>
#include <string>
#include "client.h"
#include "Logger.h"
#include "udp_server.h"
using namespace std;
extern int UDPPort;
extern bool isServer;
extern int UDPSenderSleep;
extern int UDPSenderDelay;
extern vector<client> clients;
extern Logger logger;

void processUDPSender()
{
	
	Sleep(UDPSenderDelay);
		int iResult;
		WSADATA wsaData;

		SOCKET SendSocket = INVALID_SOCKET;
		sockaddr_in RecvAddr;

		unsigned short Port = UDPPort;

		char SendBuf[1024];
		int BufLen = 1024;

		//----------------------
		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != NO_ERROR) {
			printf("WSAStartup failed with error: %d\n", iResult);
			logger.write("WSAStartup failed with error:closesocket failed with error  " + std::to_string(iResult));
			return;
		}

		//---------------------------------------------
		// Create a socket for sending data
		SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (SendSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			logger.write("socket failed with error:   " + std::to_string(WSAGetLastError()));
			WSACleanup();
			return;
		}
		//---------------------------------------------
		// Set up the RecvAddr structure with the IP address of
		// the receiver (in this example case "192.168.1.1")
		// and the specified port number.
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(Port);
		RecvAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		//RecvAddr.sin_addr.s_addr =  inet_addr("127.0.0.1"); 


		BOOL bOptVal = TRUE;
		int bOptLen = sizeof(BOOL);
		iResult = setsockopt(SendSocket, SOL_SOCKET, SO_BROADCAST, (char *)&bOptVal, bOptLen);

		//---------------------------------------------
		// Send a datagram to the receiver
		printf("Sending a datagram to the receiver (future tcp client)...\n");
		logger.write("Sending a datagram to the receiver (future tcp client)... ");
		while (1) {
			iResult = sendto(SendSocket,
				SendBuf, BufLen, 0,
				(SOCKADDR *)& RecvAddr,
				sizeof(RecvAddr));
			if (iResult == SOCKET_ERROR) {
				printf("sendto failed with error: %d\n", WSAGetLastError());
				logger.write("sendto failed with error:   " + std::to_string(WSAGetLastError()));
				closesocket(SendSocket);
				WSACleanup();
				return;
			}

			Sleep(UDPSenderSleep);
		}
		//---------------------------------------------
		// When the application is finished sending, close the socket.
		printf("Finished sending. Closing socket.\n");
		logger.write("Finished sending. Closing socket.");
		iResult = closesocket(SendSocket);
		if (iResult == SOCKET_ERROR) {
			printf("closesocket failed with error: %d\n", WSAGetLastError());
			logger.write("closesocket failed with error:  " + std::to_string(WSAGetLastError()));
			WSACleanup();
			return;
		}
		//---------------------------------------------
		// Clean up and quit.
		printf("Closing sending socket.\n");
		logger.write("Closing sending socket.");
		WSACleanup();
}