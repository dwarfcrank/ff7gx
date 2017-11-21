#include "stdafx.h"

#include <cstdio>
#include <windows.h>

static const std::size_t BUF_SIZE = 512;

void DebugLog(const char* format, ...)
{
    char buf[BUF_SIZE];

    va_list args;
    va_start(args, format);
    vsnprintf(buf, BUF_SIZE, format, args);
    va_end(args);

    OutputDebugStringA(buf);
}
