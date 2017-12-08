#pragma once

#include "Common.h"
#include <d3d9.h>

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

        const u32 TlMainVS = 0x2d180;
    }

    enum DrawType : u32
    {
        Perspective = 2,    // worldviewproj_matrix is used in the vertex shader 
        Ortho = 3           // ortho_matrix is used in the vertex shader
    };

    // TODO: Fill in the huge context struct.
    struct GameContext;

    enum GameMode
    {
        Field = 0,
        MainMenu = 3
    };

    struct GameState
    {
        u32 index;
        const char* name;
        u32 mode;
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

    struct GfxFunctions;

    GameContext* GetGameContext();
    GfxFunctions* GetGfxFunctions();

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

        IDirect3DVertexShader9* GetTlmainVS();
        void SetTlmainVS(IDirect3DVertexShader9* vs);

    private:
        Module& m_module;

        const GameState* (__cdecl *m_getGameState)();
        void (__cdecl *m_draw)(D3DPRIMITIVETYPE primType, u32 drawType, const Vertex* vertices,
            u32 vertexBufferSize, const u16* indices, u32 vertexCount, u32 a7, u32 scissor);
    };
}

