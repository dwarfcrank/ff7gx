#pragma once

#include <string>

struct Config
{
    bool loadFrida;
    std::string fridaPath;

    bool loadApitrace;
    std::string apitracePath;

    bool waitForDebugger;
};

void InitConfig();
const Config& GetConfig();
