#include "stdafx.h"

#include "log.h"
#include "Module.h"

#include <array>
#include <cstring>
#include <Windows.h>

static void PatchRaw(void* address, const void* data, u32 length)
{
    DWORD oldProtect;

    VirtualProtect(address, length, PAGE_READWRITE, &oldProtect);
    std::memcpy(address, data, length);
    VirtualProtect(address, length, oldProtect, &oldProtect);
}

Module::Module(HMODULE module) :
    m_module(module)
{
}

Module::~Module()
{
}

void Module::Patch(u32 offset, const void* data, u32 length)
{
    PatchRaw(OffsetToPtr<u8*>(offset), data, length);
}

void Module::PatchJump(u32 offset, const void* func)
{
    auto address = OffsetToPtr<u8*>(offset);

    // call <addr> is 5 bytes and the jump target is relative to the next instruction
    ptrdiff_t displacement = reinterpret_cast<const u8*>(func) - (address + 5);

    std::array<u8, 5> buf = { 0xe9, 0x00, 0x00, 0x00, 0x00 };
    *reinterpret_cast<ptrdiff_t*>(&buf[1]) = displacement;

    PatchRaw(address, buf.data(), buf.size());
}

void Module::PatchCall(u32 offset, const void* func)
{
    auto address = OffsetToPtr<u8*>(offset);

    // call <addr> is 5 bytes and the jump target is relative to the next instruction
    ptrdiff_t displacement = reinterpret_cast<const u8*>(func) - (address + 5);

    std::array<u8, 5> buf = { 0xe8, 0x00, 0x00, 0x00, 0x00 };
    *reinterpret_cast<ptrdiff_t*>(&buf[1]) = displacement;

    PatchRaw(address, buf.data(), buf.size());
}

void** Module::FindImport(const char* targetLib, const char* targetFunc)
{
    auto dosHeader = reinterpret_cast<const PIMAGE_DOS_HEADER>(m_module);

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return nullptr;
    }

    auto ntHeaders = OffsetToPtr<const PIMAGE_NT_HEADERS>(dosHeader->e_lfanew);
    auto importDesc = OffsetToPtr<const PIMAGE_IMPORT_DESCRIPTOR>(
        ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
    );

    for (auto desc = importDesc; desc->Characteristics && desc->Name; desc++) {
        auto libName = OffsetToPtr<const char*>(desc->Name);

        if (_stricmp(targetLib, libName) != 0) {
            continue;
        }

        auto firstThunk = OffsetToPtr<void**>(desc->FirstThunk);
        auto originalFirstThunk = OffsetToPtr<const PIMAGE_THUNK_DATA>(desc->OriginalFirstThunk);

        for (auto idx = 0; originalFirstThunk[idx].u1.Function; idx++) {
            auto import = OffsetToPtr<const PIMAGE_IMPORT_BY_NAME>(
                originalFirstThunk[idx].u1.AddressOfData
            );

            if (originalFirstThunk[idx].u1.Ordinal & IMAGE_ORDINAL_FLAG) {
                DebugLog("W: Function exported only by ordinal, skipping...");
            } else if (!strcmp(targetFunc, reinterpret_cast<const char*>(import->Name))) {
                DebugLog("Found %s!%s at %p", targetLib, targetFunc, &firstThunk[idx]);
                return &firstThunk[idx];
            }
        }
    }

    DebugLog("Couldn't find %s!%s", targetLib, targetFunc);
    return nullptr;
}

const void* Module::HookImport(const char* targetLib, const char* targetFunc, const void* newFunction)
{
    _ASSERTE(m_module);

    auto import = FindImport(targetLib, targetFunc);

    if (!import) {
        return nullptr;
    }

    auto oldImport = *import;
    PatchRaw(import, &newFunction, sizeof(void*));

    return oldImport;
}
