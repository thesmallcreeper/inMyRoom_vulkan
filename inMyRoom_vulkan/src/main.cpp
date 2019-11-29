#include <memory>
#include <cassert>

#include "Engine.h"

#define CONFIGURU_IMPLEMENTATION 1
#include "configuru.hpp"

#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

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

    std::unique_ptr<Engine> mainEngine_uptr = std::make_unique<Engine>(cfgFile);
    mainEngine_uptr->Run();

    return 0;
}

// Pretty lonely over here for a main.cpp, right?