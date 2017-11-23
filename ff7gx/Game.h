#pragma once

#include "common.h"

class Module;
class Renderer;

namespace FF7
{
    namespace Offsets
    {
        // Specific to ff7_en.exe
        // The base address of the game exe never changes, so these can be directly cast to pointers.
        const u32 GameContextPtr = 0xdb2bb8;

        // Specific to af3dn.p
        const u32 TextureFilteringFlag = 0x2d280;
        const u32 TileDrawCall = 0x4a48;
        const u32 DebugLogFlag = 0x2ae7c;
        const u32 RenderWidth = 0x2bc98;
        const u32 RenderHeight = 0x2bc9c;
        const u32 D3DDevice = 0x2bcc4;
        const u32 DrawFunction = 0xb6a0;
        const u32 GetGameState = 0x1330;
        const u32 DebugOverlayFlag = 0x2bc80;
    }

    enum DrawType : u32
    {
        Perspective = 2,    // worldviewproj_matrix is used in the vertex shader 
        Ortho = 3           // ortho_matrix is used in the vertex shader
    };

    // TODO: Fill in the huge context struct.
    struct GameContext;

    struct GameState
    {
        u32 index;
        const char* name;
        u32 id;
        u32 unknown;
        u32 (__cdecl *FuncPtr)(GameContext*);
    };

    struct Vertex
    {
        float x, y, z, w;
        u32 color;
        float unknown;
        float u, v;
    };

    struct GfxContext
    {
        /* 0x00 */ u32 (__cdecl *GfxFn_0)(u32 a0);
        /* 0x04 */ u32 (__cdecl *Shutdown)(u32 a0);
        /* 0x08 */ u32 dword08;
        /* 0x0C */ u32 dword0C;
        /* 0x10 */ u32 (__cdecl *EndFrame)(u32 a0);
        /* 0x14 */ u32 (__cdecl *Clear)(u32 a0, u32 a1);
        /* 0x18 */ u32 (__cdecl *ClearAll)();
        /* 0x1C */ u32 dword1C;
        /* 0x20 */ u32 dword20;

        // These fields don't seem to be used for anything, so store a Renderer instance here.
        // Probably should be the first place to check when inexplicable crashes happen.
        /* 0x24 */ Renderer* rendererInstance;
        /* 0x28 */ u32 dword28;
        /* 0x2C */ u32 dword2C;
        /* 0x30 */ u32 dword30;
        /* 0x34 */ u32 dword34;
        /* 0x38 */ u32 dword38;
        /* 0x3C */ u32 dword3C;

        /* 0x40 */ u32 dword40;
        /* 0x44 */ u32 dword44;
        /* 0x48 */ u32 dword48;
        /* 0x4C */ u32 (__cdecl *GfxFn_4C)(u32 a0);
        /* 0x50 */ void* (__cdecl *GfxFn_50)(void *a1, void *a2, void *a3);
        /* 0x54 */ u32 (__cdecl *GfxFn_54)(u32 a0, u32 a1, u32 a2, u32 a3, u32 a4);
        /* 0x58 */ u32 (__cdecl *GfxFn_58)(u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5);
        /* 0x5C */ u32 dword5C;
        /* 0x60 */ u32 dword60;
        /* 0x64 */ u32 (__cdecl *SetRenderState)(u32 a0, u32 a1, u32 a2);
        /* 0x68 */ u32 (__cdecl *GfxFn_68)(u32 a0, u32 a1);
        /* 0x6C */ u32 (__cdecl *GfxFn_6C)(u32 a0, u32 a1);
        /* 0x70 */ u32 (__cdecl *GfxFn_70)(u32 a0, u32 a1);
        /* 0x74 */ u32 (__cdecl *GfxFn_74)(u32 a0, u32 a1);
        /* 0x84 */ u32 (__cdecl *GfxFn_78)(u32 a0, u32 a1);
        /* 0x7C */ u32 (__cdecl *GfxFn_7C)(u32 a0, u32 a1);
        /* 0x80 */ u32 (__cdecl *GfxFn_80)(u32 a0, u32 a1);;
        /* 0x84 */ u32 (__cdecl *GfxFn_84)(u32 a0, u32 a1);
        /* 0x88 */ u32 (__cdecl *GfxFn_88)(u32 a0, u32 a1);
        /* 0x8C */ u32 (__cdecl *ResetState)(u32 a0);
        /* 0x90 */ u32 dword90;
        /* 0x94 */ u32 dword94;
        /* 0x98 */ u32 dword98;
        /* 0x9C */ u32 dword9C;
        /* 0xA0 */ u32 dwordA0;
        /* 0xA4 */ u32 dwordA4;
        /* 0xA8 */ void (__cdecl *DrawModel2_A8)(u32 a0, u32 a1);
        /* 0xAC */ void (__cdecl *DrawModel2_AC)(u32 a0, u32 a1);
        /* 0xB0 */ void (__cdecl *DrawModel2_B0)(u32 a0, u32 a1);
        /* 0xB4 */ void (__cdecl *DrawTiles)(void* a0, void* a1);
        /* 0xB8 */ u32 (__cdecl *GfxFn_B8)(u32 a0, u32 a1, u32 a2);
        /* 0xBC */ u32 (__cdecl *GfxFn_BC)(u32 a0, u32 a1, u32 a2);
        /* 0xC0 */ u32 (__cdecl *GfxFn_C0)(u32 a0, u32 a1, u32 a2);
        /* 0xC4 */ u32 (__cdecl *GfxFn_C4)(u32 a0, u32 a1, u32 a2);
        /* 0xC8 */ u32 (__cdecl *GfxFn_C8)(u32 a0, u32 a1, u32 a2);
        /* 0xCC */ void (__cdecl *DrawModel3_CC)(u32 a0, u32 a1);
        /* 0xD0 */ void (__cdecl *DrawModel3_D0)(u32 a0, u32 a1);
        /* 0xD4 */ void (__cdecl *DrawModel3_D4)(u32 a0, u32 a1);
        /* 0xD8 */ void (__cdecl *DrawTiles2)(void* a0, void* a1);
        /* 0xDC */ u32 dwordDC;
        /* 0xE0 */ u32 dwordE0;
        /* 0xE4 */ u32 (__cdecl *GfxFn_E4)(u32 a0, u32 a1);
        /* 0xE8 */ u32 (__cdecl *GfxFn_E8)(u32 a0, u32 a1);
        /* 0xEC */ u32 dwordEC;
    };

    GameContext* GetGameContext();
    GfxContext* GetGfxContext();

    // Accessor class for game internal data and functions, to make
    // the distinction between mod and game code more explicit
    class GameInternals
    {
    public:
        GameInternals(Module& module);
        ~GameInternals() = default;

        struct IDirect3DDevice9* GetD3DDevice();
        void GetRenderDimensions(float* width, float* height) const;
        void Draw(D3DPRIMITIVETYPE primType, u32 drawType, const Vertex* vertices,
            u32 vertexBufferSize, const u16* indices, u32 vertexCount, u32 a7, u32 scissor);

        const GameState* GetGameState() const;

        void SetDebugLogFlag(u32 value);
        void SetDebugOverlayFlag(u32 value);
        void SetTextureFilteringFlag(u32 value);

    private:
        Module& m_module;

        const GameState* (__cdecl *m_getGameState)();
        void (__cdecl *m_draw)(D3DPRIMITIVETYPE primType, u32 drawType, const Vertex* vertices,
            u32 vertexBufferSize, const u16* indices, u32 vertexCount, u32 a7, u32 scissor);
    };
}

