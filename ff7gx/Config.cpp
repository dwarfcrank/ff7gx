#include "stdafx.h"

#include "Config.h"

#include <windows.h>
#include <string>

static const CHAR INI_PATH[] = ".\\ff7gx.ini";
static const CHAR INI_SECTION[] = "ff7gx";

static Config g_config;

static std::string GetConfigString(const CHAR* key, const CHAR* defaultValue)
{
    CHAR buf[MAX_PATH];
    GetPrivateProfileString(INI_SECTION, key, defaultValue, buf, MAX_PATH, INI_PATH);

    return std::string(buf);
}

static bool GetConfigBool(const CHAR* key, bool defaultValue)
{
    UINT uintVal = GetPrivateProfileInt(INI_SECTION, key, defaultValue ? 1 : 0, INI_PATH);
    return uintVal != 0;
}

void InitConfig()
{
    g_config.loadFrida = GetConfigBool("LoadFrida", false);
    g_config.fridaPath = GetConfigString("FridaPath", "frida-gadget.dll");

    g_config.loadApitrace = GetConfigBool("LoadApitrace", false);
    g_config.apitracePath = GetConfigString("ApitracePath", "apitrace-d3d9.dll");

    g_config.waitForDebugger = GetConfigBool("WaitForDebugger", false);
}

const Config& GetConfig()
{
    return g_config;
}
