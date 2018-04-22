
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
#include<string>
#include "client.h"
#include "udp_server.h"
#include "Logger.h"
using namespace std;
extern int UDPPort;
extern bool isServer;
extern IN_ADDR server_addr;
extern vector<client> clients;
extern Logger logger;
void processUDPReceiver()
{
	//while (1)
	{
		int iResult = 0;

		WSADATA wsaData;

		SOCKET RecvSocket;
		sockaddr_in RecvAddr;

		unsigned short Port = UDPPort;

		char RecvBuf[1024];
		int BufLen = 1024;

		sockaddr_in SenderAddr;
		int SenderAddrSize = sizeof(SenderAddr);

		//-----------------------------------------------
		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != NO_ERROR) {
			printf("WSAStartup failed with error %d\n", iResult);
			logger.write("WSAStartup failed with error " + std::to_string(iResult));
			return;
		}
		//-----------------------------------------------
		// Create a receiver socket to receive datagrams
		RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (RecvSocket == INVALID_SOCKET) {
			wprintf(L"socket failed with error %d\n", WSAGetLastError());
			logger.write("socket failed with error " + std::to_string(WSAGetLastError()));
			return;
		}


		DWORD timeout = 5000;
		setsockopt(RecvSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

		//-----------------------------------------------
		// Bind the socket to any address and the specified port.
		memset(&RecvAddr, 0, sizeof(RecvAddr));

		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(Port);
		RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		iResult = ::bind(RecvSocket, (SOCKADDR *)& RecvAddr, sizeof(RecvAddr));
		if (iResult != 0) {
			wprintf(L"bind failed with error %d\n", WSAGetLastError());
			logger.write("bind failed with error " + std::to_string(WSAGetLastError()));
			return;
		}
		//-----------------------------------------------
		// Call the recvfrom function to receive datagrams
		// on the bound socket.
		printf("Receiving datagrams...\n");
		logger.write("Receiving datagrams...");
		iResult = recvfrom(RecvSocket,
			RecvBuf, BufLen, 0, (SOCKADDR *)& SenderAddr, &SenderAddrSize);
		if (iResult == SOCKET_ERROR) {

			isServer = true;

			wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
			logger.write("recvfrom failed with error " + std::to_string(WSAGetLastError()));

			closesocket(RecvSocket);
			WSACleanup();
			return;
		}

		//-----------------------------------------------
		// Close the socket when finished receiving datagrams

		printf("Datagram from: %s, port %d - it is server\n",
			inet_ntoa(SenderAddr.sin_addr), ntohs(SenderAddr.sin_port));
		logger.write("Datagram from:" + string(inet_ntoa(SenderAddr.sin_addr))+
		 ", port " + std::to_string(ntohs(SenderAddr.sin_port))+ " - it is server");
		server_addr = SenderAddr.sin_addr;
		//clients.push_back(client(SenderAddr.sin_addr, clients.size()));

		printf("Finished receiving. Closing socket.\n");
		logger.write("Finished receiving. Closing socket.");
		iResult = closesocket(RecvSocket);
		if (iResult == SOCKET_ERROR) {
			printf("closesocket failed with error %d\n", WSAGetLastError());
			logger.write("closesocket failed with error  " + std::to_string(WSAGetLastError()));
			return;
		}

		//-----------------------------------------------
		// Clean up and exit.
		printf("Closing receiving socket.\n");
		logger.write("Closing receiving socket.");
		WSACleanup();
	}
}