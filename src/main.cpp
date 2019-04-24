
#define WINVER 0x0502
#define OS_WINDOWS
#define IBPP_WINDOWS 1

#include "../include/PandoraTest.h"

/* if windows it will run the application without opening a console*/
#ifdef _WIN32
//#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

TLogger * logger = new TLogger("log.txt");
TConfig * config = new TConfig();

int main(int argc, char* argv[])
{
	std::string conf_path = "";
	bool autoconf = false;

	/*
	argv[1] should be port name
	argv[2] should be database name
	*/
 	if (argc < 3)
 	{
 		std::cout << "Need port!\n";
 		return 1;
 	}
	
	if (argc > 3)
	{
		std::string param (argv[3]);
		if (param == "-conf")
		{
			conf_path = argv[4];
		}
		else if (param == "autoconf")
		{
			autoconf = true;
		}
	}

	PandoraTest * pandora = new PandoraTest(argv[1], argv[2], conf_path, autoconf);

	SAFEDELETE(pandora)
	SAFEDELETE(logger)
	SAFEDELETE(config)
	return 0;

}



/*
compilation error xtime.hpp
https://svn.boost.org/trac10/ticket/6940
https://stackoverflow.com/questions/17599377/boost-error-trouble-compiling-xtime-hpp
fix:
You can edit /usr/include/boost/thread/xtime.hpp (or whereever your xtime.hpp lies) 
and change all occurrences of TIME_UTC to TIME_UTC_ - Probably around lines 23 and 71.
*/

/*
https://stackoverflow.com/questions/53771521/errors-while-compiling-old-c-project-using-boost-1-46-with-vs-2015/53771595#53771595
compilation error in win_iocp_io_service.ipp

Adding include algorithm  to detail/impl/win_iocp_io_service.ipp fixed the issue.

Why it happened with VS15 is still a mystery to me


*/

/*
The ifstream class has an operator void *() (or operator bool() in C++11).\
testing for a NULL should be done by calling function fin.fail().
*/