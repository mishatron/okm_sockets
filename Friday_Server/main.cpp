// udp_server.cpp 

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
#include <chrono>
#include "Logger.h"
#include "tcp_server.h"
#include "tcp_client.h"
#include "client.h"
#include "udp_client.h"
#include "udp_server.h"
// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
using namespace std;
#define pb push_back
// block constants
 int UDPSocketTimeout = 5000;
 int UDPSenderSleep = 100;
 int UDPSenderDelay = 6000;
 int UDPPort = 27015;
 IN_ADDR server_addr;
// block constants
bool isServer = false;
Logger logger;
int sec = 0;
void timer()
{
	cout << "time=" << sec << endl;
	sec++;
	Sleep(998);
	if (sec < 5)
		timer();
}



int main()
{
	thread receiveUDP(processUDPReceiver);
	//thread timer(timer);

	//timer.join();
	receiveUDP.join();


	if (isServer)
	{
		thread sendUDP(processUDPSender);
		//tcp_serve();
		thread server(tcp_serve);
		sendUDP.join();
		server.join();
	}
	else
	{
		thread client(tcp_client);
		client.join();
	}


	system("pause");
	return 0;
}