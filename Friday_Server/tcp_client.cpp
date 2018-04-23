// myclient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <conio.h>
#include <winsock.h>
#include "tcp_client.h"
#include "Logger.h"
#include <string>
#include<thread>
#include <iostream>
// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define SERVER_PORT2  8001
char bufer[200];
char bufer2[200];
extern IN_ADDR server_addr;
extern Logger logger;
bool exitFlag = 0;
CRITICAL_SECTION csExit;
int readlineC(SOCKET c, char *buf)
{
	int rez, num = 0;
	char *start = buf;

	while (1)
	{

		EnterCriticalSection(&csExit);
		if (exitFlag) break;
		LeaveCriticalSection(&csExit);

		rez = recv(c, buf, 1, 0);

		if (rez>0)
		{
			num++;

			if (*buf == '\n')break;

			buf++;
		}
		else if (rez == 0)
		{
			printf("Socket closed!\n");
			logger.write("Socket closed!  ");

			EnterCriticalSection(&csExit);
			exitFlag = 1;
			LeaveCriticalSection(&csExit);
			return 0;
		}
		else
		{
			switch (WSAGetLastError())
			{
			case WSAEWOULDBLOCK: // нічого не прийшло
				//Next();
				break;
			default:  //інші помилки
				printf("Error!!! %d\n", WSAGetLastError());
				logger.write("Error!!! " + std::to_string(WSAGetLastError()));

				EnterCriticalSection(&csExit);
				exitFlag = 1;
				LeaveCriticalSection(&csExit);

				return 0;
			}
		}




	}

	*(++buf) = '\0';

	return num;

}

int writelineC(SOCKET c, char *buf, int maxlen)
{
	int num = 0;

	while (send(c, buf, 1, 0) == 1)
	{
		num++;

		if (num == maxlen)break;

		if (*buf == '\n')break;

		buf++;

	}

	return num;
}

void readFromKeyboard(SOCKET ms)
{
	int len = 0;
	do
	{
		fgets(bufer2, 200, stdin);

		if (strcmp(bufer2, "quit\n") == 0) {
			logger.write("quit");
			EnterCriticalSection(&csExit);
			exitFlag = 1;
			LeaveCriticalSection(&csExit);
			return;
		}
		if ((len = strlen(bufer2)) == 1)continue;

		writelineC(ms, bufer2, len);
	} while (1);
}
int tcp_client()
{
	SOCKET ms;
	int len;
	char adrbuf[200];


	SOCKADDR_IN serv_addr;
	int serv_addr_len;

	setlocale(LC_ALL, "Ukrainian");

	InitializeCriticalSection(&csExit);
	//--------------------------
	WORD wVersionRequested;
	WSADATA wsaData;
	int err, r;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return -1;
	}
	//--------------------------

	unsigned long serv_ip_address = inet_addr(inet_ntoa(server_addr));

	ms = socket(AF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(SOCKADDR_IN));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.S_un.S_addr = serv_ip_address;
	serv_addr.sin_port = htons(SERVER_PORT2);

	if (connect(ms, (SOCKADDR *)&serv_addr, sizeof(serv_addr)) != 0)
	{
		printf("Error %d connecting server :( \n", WSAGetLastError());
		logger.write("Error connecting server :(   " + std::to_string(WSAGetLastError()));
		exit(-2);
	}

	//Non-blocking mode
	unsigned long ulMode;
	ulMode = 1;
	ioctlsocket(ms, FIONBIO, (unsigned long*)&ulMode);

	printf("З'єднання: %s, сокет %d \n",
		inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));


	logger.write("connection:  " + std::string(inet_ntoa(serv_addr.sin_addr)) +
		", socket: " + std::to_string(ntohs(serv_addr.sin_port)));

	//enterning name
	char q[] = "name ";
	printf("enter name:");
	char res[200];
	memset(res, '\0', sizeof(res));
	strcpy(res, q);
	fgets(bufer2, 200, stdin);
	strcat(res, bufer2);
	writelineC(ms, res, strlen(res));
	//

	std::thread reader(readFromKeyboard, ms);
	reader.detach();
	do
	{

		EnterCriticalSection(&csExit);
		if (exitFlag) break;
		LeaveCriticalSection(&csExit);

		len = readlineC(ms, bufer);
		if (len > 1)
		{
			printf("%d bytes received:%s\n", len, bufer);
			logger.write(std::to_string(len) + " bytes received ");
		}
	} while (1);

	closesocket(ms);
	logger.write("closing socket on client");
	return 0;
}

