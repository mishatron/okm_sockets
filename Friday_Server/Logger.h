#include <fstream>
#include <time.h>
#include<chrono>
class Logger
{
	std::ofstream fout;
public:
	Logger()
	{
		fout.open("log.txt", std::ios::app);
	}
	void write(std::string msg)
	{

		time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		fout << ctime(&t) << " --- " << msg.c_str() << std::endl;
	}
	~Logger()
	{
		fout.close();
	}
	
};