// Friday_Server.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <iostream>
#include <conio.h>
#include <winsock.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

int global_flag = 0;

#define SERVER_PORT  8001
#define LISTENQ 1024
#define ARRSIZE 10
#define MAXFIBNUMBER 100
char info_names[ARRSIZE][50];

SOCKET ms;
SOCKET conn;
SOCKADDR_IN serv_addr, client_addr;
int client_addr_len, len;

char gmes1[1000];

HANDLE hThread;
HANDLE hThread2;
CRITICAL_SECTION csSerializeDraw;
//--------------------------

class TCPBuf
{
	SOCKET s;
	int used; // 0 - not  1 - yes
	char inbuf[2000];  int inbytes;
	char outbuf[2000]; int outbytes;
	char tmpbuf[2000];
public:
	TCPBuf();
	int SendRecv();
	void LinkSocket(SOCKET as);
	void UnlinkSocket();
	void GetPointersAndLens(char **pinbuf, char **poutbuf, int *pinlen, int *poutlen);
	void GetInbuf(int dellen);
	int PutOutbuf(char *what, int putlen);
	int isUsed(){ return used; }
};

void TCPBuf::GetPointersAndLens(char **pinbuf,
	char **poutbuf,
	int *pinlen,
	int *poutlen)
{
	*pinbuf = inbuf;
	*poutbuf = outbuf;
	*pinlen = inbytes;
	*poutlen = outbytes;
}

void TCPBuf::GetInbuf(int dellen)
{
	if (dellen == inbytes)inbytes = 0;
	else
	{
		memcpy(tmpbuf, &inbuf[dellen], inbytes - dellen);
		memcpy(inbuf, tmpbuf, inbytes - dellen);

		inbytes -= dellen;
	}
}

int TCPBuf::PutOutbuf(char *what, int putlen)
{
	if (outbytes + putlen > sizeof(outbuf))return -1;

	memcpy(&outbuf[outbytes], what, putlen);

	outbytes += putlen;

	return 0;
}


class TCPBuffers
{
	TCPBuf arr[ARRSIZE];
public:
	int AddSocket(SOCKET as);
	void DeleteSockets();
	void DeleteOneSocket(int n);
	void ReadWriteSockets();
	int getClientData(char *buf, int *pnum);
	int putClientData(int num, char *buf);
	int getRandomConnection();
	void AddToAll(char *);

	bool isSocketUsed(int n) 
	{
		return arr[n].isUsed();
	}
};

TCPBuffers tcpbuffers;

int TCPBuffers::getRandomConnection()
{
	int num = 0, which;


	for (int i = 0; i<ARRSIZE; i++)
	{
		if (arr[i].isUsed())num++;
	}

	if (num == 0)return -1;

	which = rand() % num;
	num = 0;

	for (int i = 0; i<ARRSIZE; i++)
	{
		if (arr[i].isUsed())
		{
			if (num == which)return i;
			num++;
		}
	}

	return -1;
}

int TCPBuffers::putClientData(int num, char *buf)
{
	if (!arr[num].isUsed())return 0;

	arr[num].PutOutbuf(buf, strlen(buf));
}

int TCPBuffers::getClientData(char *buf, int *pnum)
{
	char *pinbuf, *poutbuf;
	int inlen, outlen;

	for (int i = 0; i<ARRSIZE; i++)
	{
		if (arr[i].isUsed())
		{
			arr[i].GetPointersAndLens(&pinbuf, &poutbuf,
				&inlen, &outlen);

			for (int j = 0; j<inlen; j++)
			{
				buf[j] = pinbuf[j];
				if (buf[j] == '\n')
				{
					*pnum = i;
					buf[j] = '\0';
					arr[i].GetInbuf(j + 1);
					return 1;
				}
			}
		}
	}

	return 0;
}

void TCPBuffers::DeleteOneSocket(int n)
{
	if (arr[n].isUsed())
		arr[n].UnlinkSocket();
}

void TCPBuffers::DeleteSockets()
{
	for (int i = 0; i<ARRSIZE; i++)
	{
		if (arr[i].isUsed())arr[i].UnlinkSocket();
	}
}

void TCPBuffers::AddToAll(char *sbuf)
{
	for (int i = 0; i<ARRSIZE; i++)
	{
		if (arr[i].isUsed())
		{
			putClientData(i, sbuf);
		}
	}
}

void TCPBuffers::ReadWriteSockets()
{
	for (int i = 0; i<ARRSIZE; i++)
	{
		if (arr[i].isUsed())
		{
			if (arr[i].SendRecv())
			{
				arr[i].UnlinkSocket();
			}
		}
	}
}

int TCPBuffers::AddSocket(SOCKET as)
{
	for (int i = 0; i < ARRSIZE; i++)
	{
		if (!arr[i].isUsed())
		{
			arr[i].LinkSocket(as);
			return i;
		}
	}
	return -1;
}

TCPBuf::TCPBuf()
{
	used = 0;
}

void TCPBuf::UnlinkSocket()
{
	closesocket(s);
	used = 0;
}

void TCPBuf::LinkSocket(SOCKET as)
{
	s = as;
	used = 1;
	inbytes = 0;
	outbytes = 0;
}

int TCPBuf::SendRecv()
{
	int err;

	if (outbytes>0)
	{

		//outbuf[outbytes]='\0';
		//printf("Sent %s", outbuf);
		err = send(s, outbuf, outbytes, 0);

		if (err != SOCKET_ERROR)
		{
			if (outbytes == err)
			{
				outbytes = 0;
			}
			else
			{
				memcpy(tmpbuf, &outbuf[err], outbytes - err);
				memcpy(outbuf, tmpbuf, outbytes - err);
				outbytes -= err;
			}
		}
		else
		{
			err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)return -1;
		}
	}
	//--------------------------------------
	err = recv(s, tmpbuf, 1000 - inbytes, 0);

	if (err>0)
	{
		memcpy(&inbuf[inbytes], tmpbuf, err);
		inbytes += err;
	}
	else
	{
		if (err == 0)return -1;
		else
		{
			err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)return -1;
		}
	}

	return 0;
}

long WINAPI ThreadRoutine2(LPVOID lpParameter)
{
	int flag = 1;
	char buf[1001];
	char buf2[1001];
	int cnum;

	int amove_dx, amove_dy;
	char amove_buf[100];
	char name_buf[100];

	do
	{
		EnterCriticalSection(&csSerializeDraw);
		flag = global_flag;
		LeaveCriticalSection(&csSerializeDraw);

		if (flag == 0)break;

		EnterCriticalSection(&csSerializeDraw);
		if (tcpbuffers.getClientData(buf, &cnum))
		{
			//if (sscanf(buf, "%s%d%d", amove_buf,
			//	&amove_dx, &amove_dy) == 3)
			//{
			//	//game.MoveAgent(cnum, amove_dx, amove_dy);
			//}

			if (strncmp(buf, "NAME", 4) == 0)
			{
				if (sscanf(buf, "%s%s", amove_buf, name_buf))
				{
					sprintf(&info_names[cnum][0], "%s", name_buf);

				}
			}
			else if (strncmp(buf, "STAT", 4) == 0)
			     {
				    sprintf(buf, "STAT ");
					for (int i = 0; i < ARRSIZE; i++)
					{
						if (tcpbuffers.isSocketUsed(i))
						{
							sprintf(buf2, "%s:", &info_names[i][0]);
							strcat(buf, buf2);
						}
					}
					strcat(buf, "\n");
					tcpbuffers.putClientData(cnum, buf);

			     }
		}
		LeaveCriticalSection(&csSerializeDraw);

		Sleep(100);

	} while (1);

	return (TRUE);
}

long WINAPI ThreadRoutine(LPVOID lpParameter)
{
	int flag = 1, client_num;
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	//--------------------------
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return -1;
	}
	//--------------------------

	ms = socket(AF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(SOCKADDR_IN));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERVER_PORT);

	bind(ms, (SOCKADDR *)&serv_addr, sizeof(serv_addr));

	//-----
	unsigned long    blockmode = 1;
	int r;
	r = ioctlsocket(ms, FIONBIO, &blockmode);
	if (r == 0)
	{
		r = WSAGetLastError();
	}

	//------

	listen(ms, LISTENQ);


	do
	{
		EnterCriticalSection(&csSerializeDraw);
		flag = global_flag;
		LeaveCriticalSection(&csSerializeDraw);

		if (flag == 0)break;

		client_addr_len = sizeof(client_addr);
		conn = accept(ms, (SOCKADDR *)&client_addr, &client_addr_len);

		if (conn == INVALID_SOCKET)
		{
			err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)break;


		}
		else
		{
			EnterCriticalSection(&csSerializeDraw);
			if ((client_num = tcpbuffers.AddSocket(conn)) == -1)
				closesocket(conn);

			sprintf(&info_names[client_num][0], "Client %d", client_num);


			//game.AddUser(client_num);
			//refresh_object.RefreshAll();

			LeaveCriticalSection(&csSerializeDraw);
		}

		//TryEnterCriticalSection(&csSerializeDraw);
		EnterCriticalSection(&csSerializeDraw);
		int k;

		//while ((k = killer.GetNext()) != -1)
		//{
		//	tcpbuffers.DeleteOneSocket(k);
		//}
		LeaveCriticalSection(&csSerializeDraw);

		EnterCriticalSection(&csSerializeDraw);
		tcpbuffers.ReadWriteSockets();
		LeaveCriticalSection(&csSerializeDraw);

		Sleep(100);

	} while (1);

	EnterCriticalSection(&csSerializeDraw);
	tcpbuffers.DeleteSockets();
	LeaveCriticalSection(&csSerializeDraw);

	return (TRUE);
}

char sendBuffer[2000];

#define INFO_SIZE 10000
double info_buf[INFO_SIZE] = { 1.0, 1.0 };
int info_index = 2;

double x1 = 1.0;
double x2 = 1.0;
double x3 ;

int main2(int argc, char* argv[])
{
	int some_parameter;
	int dwIDThread;
	int some_parameter2;
	int dwIDThread2;

	for (int i = 0; i < ARRSIZE; i++)
		sprintf(&info_names[i][0], "Client_%d", i);

	InitializeCriticalSection(&csSerializeDraw);

	global_flag = 1;

	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadRoutine, (LPVOID)&some_parameter,
		0, (LPDWORD)&dwIDThread);


	hThread2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadRoutine2, (LPVOID)&some_parameter2,
		0, (LPDWORD)&dwIDThread2);

	char ch;
	int n, k;

	do{
		if (_kbhit())
		{
			ch = _getche();
			if (ch == 27)  //ESC
			{
				EnterCriticalSection(&csSerializeDraw);
				tcpbuffers.DeleteSockets();
				global_flag = 0;
				LeaveCriticalSection(&csSerializeDraw);
				break;
			}
		}

		EnterCriticalSection(&csSerializeDraw);
		//game.Day();
		x3 = x2 + x1;
		if (info_index < INFO_SIZE)
		{
			info_buf[info_index++] = x3;
			x1 = x2;
			x2 = x3;
		}
		sprintf(sendBuffer, "INFO %lf\n", x3);
		tcpbuffers.AddToAll(sendBuffer);

		LeaveCriticalSection(&csSerializeDraw);

		Sleep(1000);

		EnterCriticalSection(&csSerializeDraw);

		//if (refresh_object.Get_Refresh(sendBuffer))
		//{
		//	tcpbuffers.AddToAll(sendBuffer);
		//	refresh_object.Reset_Refresh();
		//}

		LeaveCriticalSection(&csSerializeDraw);

	} while (true);

	EnterCriticalSection(&csSerializeDraw);
	global_flag = 0;
	LeaveCriticalSection(&csSerializeDraw);

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);

	WaitForSingleObject(hThread2, INFINITE);
	CloseHandle(hThread2);

	return 0;
}

