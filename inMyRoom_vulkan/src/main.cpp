#include <memory>
#include <cassert>

#include "Engine.h"

#define CONFIGURU_IMPLEMENTATION 1
#include "configuru.hpp"

#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

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
    mainEngine.Run();

    return 0;
}

// Pretty lonely over here for a main.cpp, right?