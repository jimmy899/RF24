
extern "C" {
	unsigned long micros(void);
}

using namespace std;

class SerialConsole 
{
	public:
	void write(const char* str);
	void print(int i);
};

