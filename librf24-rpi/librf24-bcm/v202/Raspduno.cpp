#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/time.h>

#include "Raspduno.h"

using namespace std;

void SerialConsole::write(const char* str) 
{
		printf("%s", str);
};

void SerialConsole::print(int i) 
{
		printf("%d", i);
};

extern "C" {

unsigned long micros(void)
{
	unsigned long ret;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ret = tv.tv_sec * 1000000 + tv.tv_usec;
	
	return ret;
}

}

