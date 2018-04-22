#pragma once
#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
//#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <thread>
#include <sys/types.h> 
#include<iostream>
#include<vector>
//using namespace std;
class client
{
public:
	std::string name;
	IN_ADDR ip;
	USHORT port;

	client(IN_ADDR p, USHORT po, int i)
	{
		ip = p;
		port = po;
		name = "Client"/* + (i + '0')*/;
	}
	void setName(int i)
	{
		name = "Client" + (i + '0');
	}
	void setName(std::string n)
	{
		name = n;
	}

};