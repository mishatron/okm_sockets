// myserver.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <locale.h>
//#include <winsock.h>
#include "client.h"
#include <vector>
#include <thread>
#include "Logger.h"
#include <string>

//using namespace std;
//02.05 11-00
// Link with ws2_32.lib
//#pragma comment(lib, "Ws2_32.lib")


#define SERVER_PORT  8001
#define LISTENQ 1024

char buf[200];

#define OUTD   1000000
#define OUTLEN   60
char outstr[OUTLEN];
int outstr_x = 0;
int outstr_d = OUTD;
std::vector<SOCKET> clientSockets(10);
std::vector < std::thread > threads(10);
std::vector<client> clients;
int curClientSocket = 0;
bool isPare = true;
extern Logger logger;
void Next()
{
	outstr_d--;
	if (outstr_d != 0)return;

	outstr_d = OUTD;

	outstr[outstr_x] = ' ';
	outstr_x++;
	if (outstr_x > (OUTLEN - 2)) outstr_x = 0;

	outstr[outstr_x] = '#';

	printf("\r%s", outstr);
}

int readline(SOCKET c, char *buf)
{
	int rez, num = 0;
	char *start = buf;

	while (1)
	{
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
			logger.write("Socket closed!  " );
			return 0;
		}
		else
		{
			switch (WSAGetLastError())
			{
			case WSAEWOULDBLOCK: // нічого не прийшло
				Next();
				break;
			default:  //інші помилки
				printf("Error!!! %d\n", WSAGetLastError());
				logger.write("Error!!! " + std::to_string(WSAGetLastError()));
				return 0;
			}
		}




	}

	*(++buf) = '\0';

	return num;

}

int writeline(SOCKET c, char *buf, int maxlen)
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
void workWithSocket(int index)
{

	int client_addr_len, len;

	while ((len = readline(clientSockets[index], buf)) != 0)
	{
		std::cout <<"\r"<< len << " bytes received from " << (clients[index].name).c_str() << ": \n" << buf;
		logger.write("\r "+std::to_string(len)+ " bytes received from " + 
			clients[index].name.c_str()+ ":" + buf);
		//printf("\r%d bytes received from %s :\n%s", len, clients[index].name, buf);
		if (strncmp(buf, "stat",4)==0)//ready
		{
			logger.write("command stat");
			std::string res = "stat ";
			for (int i = 0; i < curClientSocket; ++i)
			{
				res += ":" + clients[i].name;
			}
			res += "\n";
			writeline(clientSockets[index], (char *)res.c_str(), res.size());
		}
		else if (strncmp(buf, "name", 4) == 0)//ready
		{

			logger.write("command name");
			std::string name = buf;
			name = name.substr(5);
			name.pop_back();
			clients[index].setName(name);
			writeline(clientSockets[index], buf, len);
		}
		else if (strncmp(buf, "kill", 4) == 0)//ready
		{
			logger.write("command kill");
			std::string name = buf;
			name = name.substr(5);
			name.pop_back();
			for (int i = 0; i < curClientSocket; ++i)
			{
				if (clients[i].name == name)
				{
					clients[i].clear();
					closesocket(clientSockets[i]);
					break;
				}
			}
		}
		else if (strncmp(buf, "bcst", 4) == 0)//ready
		{
			logger.write("command bcst");
			std::string bcst = buf;
			bcst = bcst.substr(5);
			for (int i = 0; i<curClientSocket; ++i)
				writeline(clientSockets[i], (char *)bcst.c_str(), bcst.size());
		}
		else if (strncmp(buf, "mesg", 4) == 0)//ready
		{
			logger.write("command mesg");
			std::string mesg = buf;
			mesg = mesg.substr(5);
			std::string name = "";
			int i = 0;
			for (;i<mesg.size();++i)
			{
				if (mesg[i] != ':')
					name += mesg[i];
				else break;
			}
			std::string text = "";
			for (; i < mesg.size(); ++i)
				text += mesg[i];
			text += "\n";
			for (int i = 0; i < curClientSocket; ++i)
			{
				if (clients[i].name == name)
				{
					writeline(clientSockets[i], (char *)text.c_str(), text.size());
					//break;
				}
			}
		}
		else if (strncmp(buf, "pare", 4) == 0)//ready
		{
			isPare = !isPare;
			if (isPare)
			{
				std::string res = "eho resumed\n";
				logger.write("command pare - eho resumed");
				writeline(clientSockets[index], (char*)res.c_str(), res.size());
			}
			else
			{

				std::string res = "eho paused\n";
				logger.write("command pare - eho paused");
				writeline(clientSockets[index], (char*)res.c_str(), res.size());
			}
		}
		else//eho
		{
			if (isPare)
				writeline(clientSockets[index], buf, len);
			else

				writeline(clientSockets[index], "\n", 1);
		}
	}
	
}

int tcp_serve()
{
	SOCKET ms;
	SOCKADDR_IN serv_addr, client_addr;
	int client_addr_len, len;
	//--------------------------
	WORD wVersionRequested;
	WSADATA wsaData;
	int err, r;
	setlocale(LC_ALL, "Ukrainian");

	wVersionRequested = MAKEWORD(2, 2);

	for (int i = 0; i<OUTLEN; i++)outstr[i] = ' ';
	outstr[OUTLEN - 1] = '\0';


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
	//unsigned long    blockmode = 0;              
	//r = ioctlsocket(ms, FIONBIO, NULL);     
	//r=WSAGetLastError(); 
	//------
	std::cout << "start tcp listen"<<std::endl;
	logger.write("start tcp listen");
	//WSAGetLastError();
	while (curClientSocket < 10)
	{

		listen(ms, LISTENQ);
		client_addr_len = sizeof(client_addr);
		clientSockets[curClientSocket] = accept(ms, (SOCKADDR *)&client_addr, &client_addr_len);

		if (clientSockets[curClientSocket] < 0) {
			printf("Error %d\n", WSAGetLastError());
			logger.write("Error:  " + std::to_string(WSAGetLastError()));
			continue;
			exit(-1);
		}

		//Non-blocking mode
		unsigned long ulMode;
		ulMode = 1;
		ioctlsocket(clientSockets[curClientSocket], FIONBIO, (unsigned long*)&ulMode);


		printf("З'єднання: %s, порт %d\n",
			inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		logger.write("connection:  " + std::string(inet_ntoa(client_addr.sin_addr))+
			", port: "+ std::to_string(ntohs(client_addr.sin_port)));

		clients.push_back(client(client_addr.sin_addr, client_addr.sin_port, curClientSocket));
		printf("%s", clients[curClientSocket].name);
		//
		// пОРТ СОКЕТА CONN СЕРВЕРА ???
		//

		std::thread t(workWithSocket, curClientSocket);
		threads[curClientSocket] = std::move(t);
		threads[curClientSocket].detach();

		curClientSocket++;
	}
	closesocket(ms);
	for (auto s : clientSockets)
	{
		closesocket(s);
	}
	logger.write("closing server sockets");
	return 0;
}


