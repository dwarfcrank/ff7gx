#include "stdafx.h"

#include "Common.h"
#include "Config.h"
#include "Game.h"
#include "Log.h"
#include "Module.h"
#include "Renderer.h"

#include <windows.h>
#include <cstring>
#include <cstdio>

#define DLLEXPORT extern "C" __declspec(dllexport)

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

namespace FF7
{
    struct GfxFunctions;
}

using fn_new_dll_graphics_driver = FF7::GfxFunctions* (__cdecl *)(u32);
using fn_dotemuRegOpenKeyExA = u32 (__stdcall *)(u32, u32, u32, u32, u32);
using fn_dotemuRegQueryValueExA = u32 (__stdcall *)(u32, const char*, u32, u32, void*, u32*);
using fn_dotemuRegSetValueExA = u32 (__stdcall *)(u32, u32, u32, u32, u32, u32);

static Module g_originalDll;

static bool g_initialized = false;
static HMODULE g_fridaDll = nullptr;
static HMODULE g_apitraceDll = nullptr;

static void DoInit()
{
    InitConfig();

    if (GetConfig().waitForDebugger) {
        while (!IsDebuggerPresent()) {
            Sleep(500);
        }
    }

    if (GetConfig().loadFrida) {
        g_fridaDll = LoadLibrary(GetConfig().fridaPath.c_str());
        DisableThreadLibraryCalls(g_fridaDll);

        DebugLog("Loaded frida at %p", g_fridaDll);
    }

    g_originalDll = Module(LoadLibrary("AF3DN2.P"));
    DebugLog("Loaded original at %p", g_originalDll);

    if (GetConfig().loadApitrace) {
        g_apitraceDll = LoadLibrary(GetConfig().apitracePath.c_str());

        auto apitraceFunc = static_cast<const void*>(GetProcAddress(g_apitraceDll, "Direct3DCreate9"));
        g_originalDll.HookImport("d3d9.dll", "Direct3DCreate9", apitraceFunc);

        // HACK: we use D3DPERF_* stuff, so we need to hook those too
        // It'd be cleaner to just declare pointers and GetProcAddress them
        auto thisDll = Module(GetModuleHandle("AF3DN.P"));
        apitraceFunc = static_cast<const void*>(GetProcAddress(g_apitraceDll, "D3DPERF_BeginEvent"));
        thisDll.HookImport("d3d9.dll", "D3DPERF_BeginEvent", apitraceFunc);
        apitraceFunc = static_cast<const void*>(GetProcAddress(g_apitraceDll, "D3DPERF_EndEvent"));
        thisDll.HookImport("d3d9.dll", "D3DPERF_EndEvent", apitraceFunc);

        DebugLog("Loaded apitrace at %p", g_apitraceDll);
    }

    DebugLog("Init done");

    // Enable debug logging
    FF7::GameInternals internals(g_originalDll);
    internals.SetDebugLogFlag(1);

    g_initialized = true;
}

static u32 __cdecl Shutdown(u32 a0)
{
    auto instance = FF7::GetGfxFunctions()->rendererInstance;
    auto ret = instance->Shutdown(a0);

    delete instance;

    return ret;
}

void Initialize()
{
    if (g_initialized) {
        return;
    }

    DoInit();
}

DLLEXPORT FF7::GfxFunctions* __cdecl new_dll_graphics_driver(u32 a0)
{
    Initialize();

    auto function = reinterpret_cast<fn_new_dll_graphics_driver>(g_originalDll.GetExport(__func__));
    auto functions = function(a0);
    auto renderer = new Renderer(g_originalDll, functions);

    functions->rendererInstance = renderer;
    renderer->GetFunctions()->Shutdown = Shutdown;

    FF7::GameInternals internals(g_originalDll);
    internals.SetDebugOverlayFlag(1);

    return renderer->GetFunctions();
}

DLLEXPORT u32 __stdcall dotemuRegCloseKey(u32)
{
    Initialize();
    return 0;
}

DLLEXPORT u32 __stdcall dotemuRegOpenKeyExA(u32 a0, u32 a1, u32 a2, u32 a3, u32 a4)
{
    Initialize();
    auto function = reinterpret_cast<fn_dotemuRegOpenKeyExA>(g_originalDll.GetExport(__func__));
    return function(a0, a1, a2, a3, a4);
}

DLLEXPORT u32 __stdcall dotemuRegDeleteValueA(u32, u32)
{
    Initialize();
    return 0;
}

DLLEXPORT u32 __stdcall dotemuRegSetValueExA(u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5)
{
    Initialize();
    auto function = reinterpret_cast<fn_dotemuRegSetValueExA>(g_originalDll.GetExport(__func__));
    return function(a0, a1, a2, a3, a4, a5);
}

DLLEXPORT u32 __stdcall dotemuRegQueryValueExA(u32 a0, const char* valueName, u32 a2, u32 a3,
                                               void* data, u32* dataSize)
{
    Initialize();

    // Enable debug logging in the game
    if (!strcmp(valueName, "SSI_DEBUG")) {
        strcpy_s(static_cast<char*>(data), 16, "SHOWMETHEAPPLOG");
        *dataSize = 16;
        return 0;
    }

    auto function = reinterpret_cast<fn_dotemuRegQueryValueExA>(g_originalDll.GetExport(__func__));
    return function(a0, valueName, a2, a3, data, dataSize);
}
