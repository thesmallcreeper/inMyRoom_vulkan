#include <memory>
#include <cassert>

#include "Engine.h"

#include "configuru.hpp"

int main()
{
    configuru::Config cfgFile;

    cfgFile = configuru::parse_file("config.cfg", configuru::CFG);

    std::unique_ptr<Engine> mainEngine_uptr = std::make_unique<Engine>(cfgFile);
    mainEngine_uptr->Run();

    return 0;
}

// Pretty lonely over here for a main.cpp, right?