#pragma once

class Engine;

class EnginesExportedFunctions
{
public:
    // Using virtual functions engine properties can be tweaked/exported


private:
    friend class Engine;
    Engine* engine_ptr;
};

