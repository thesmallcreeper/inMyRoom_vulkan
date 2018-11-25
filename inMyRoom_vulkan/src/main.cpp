#include <memory>

#include "Engine.h"

#define CONFIGURU_IMPLEMENTATION 1
#include "configuru.hpp"

int main()
{
	configuru::Config cfgFile;
	try
	{
		cfgFile = configuru::parse_file("config.cfg", configuru::CFG);
	}
	catch (...)
	{
		printf("Couldn't open \"config.cfg\"\nClosing...\n");
		return -1;
	}

	Engine mainEngine(cfgFile);
	mainEngine.init();
	mainEngine.run();

	return 0;
}