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

    cfgFile = configuru::parse_file("config.cfg", configuru::CFG);

    std::unique_ptr<Engine> mainEngine_uptr = std::make_unique<Engine>(cfgFile);
    mainEngine_uptr->Run();

    return 0;
}

// Pretty lonely over here for a main.cpp, right?