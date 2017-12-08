#pragma once

#include <utility>
#include <Windows.h>
#include <d3d9.h>

// Helper class to visibly separate different parts of the rendering code
// when running in a graphics debugger
class ScopedD3DEvent
{
public:
    ScopedD3DEvent(const WCHAR* format, ...)
    {
        WCHAR buf[128];

        va_list args;
        va_start(args, format);
        std::vswprintf(buf, 128, format, args);
        va_end(args);

        D3DPERF_BeginEvent(0, buf);
    }

    ~ScopedD3DEvent()
    {
        D3DPERF_EndEvent();
    }
};
