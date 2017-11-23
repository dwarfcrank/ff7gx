#pragma once

#include "common.h"

#include <Windows.h>
#include <type_traits>

class Module
{
public:
    Module(HMODULE module = nullptr);
    ~Module();

    HMODULE GetHandle()
    {
        return m_module;
    }

    template<typename T>
    T OffsetToPtr(u32 offset)
    {
        static_assert(std::is_pointer<T>(), "T must be a pointer type");

        auto base = reinterpret_cast<u8*>(m_module);
        return reinterpret_cast<T>(base + offset);
    }

    void* GetExport(const char* name);

    const void* HookImport(const char* libName, const char* funcName, const void* newFunction);

    void Patch(u32 offset, const void* data, u32 length);
    void PatchJump(u32 offset, const void* func);
    void PatchCall(u32 offset, const void* func);

private:
    void** FindImport(const char* libName, const char* funcName);

    HMODULE m_module;
};

