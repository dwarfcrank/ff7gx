#pragma once

#include "Game.h"

#include <d3d9.h>
#include <functional>
#include <memory>
#include <Windows.h>
#include <wrl.h>

class Renderer
{
public:
    using ShutdownCallback = std::function<void()>;

    Renderer(class Module& module, FF7::GfxContext* context, ShutdownCallback shutdownCallback = nullptr);

    u32 EndFrame(u32 a0);
    u32 Clear(u32 clearRenderTarget, u32 clearDepthBuffer);
    u32 ClearAll();
    void DrawTiles(void* a0, void* a1);

    // DrawTilesImpl is patched to call this instead of the original Draw()
    void DrawHook(D3DPRIMITIVETYPE primType, u32 drawType, const FF7::Vertex* vertices,
        u32 vertexBufferSize, const u16* indices, u32 vertexCount, u32 a7, u32 scissor);

    // This needs to be static as it deletes the entire renderer instance
    static u32 __cdecl Shutdown(u32 a0);

    Renderer(Renderer&) = delete;
    Renderer(Renderer&&) = delete;

private:
    // Sets the flag used by the pixel shader to determine whether to sample from a texture.
    void SetShaderTextureFlag(bool value);

    // Sets the flag used by Draw() to determine whether to enable linear filtering or not.
    void SetTextureFilteringFlag(bool value);

    void InitViewport();
    void InitProjectionMatrix();

    ShutdownCallback m_shutdownCallback;

    // Game internals
    class Module& m_originalDll;
    FF7::GfxContext m_originalContext;
    FF7::GameInternals m_internals;

    // D3D resources
    template<typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<IDirect3DDevice9> m_d3dDevice;
    ComPtr<IDirect3DTexture9> m_backgroundTexture;
    ComPtr<IDirect3DSurface9> m_backgroundRenderTarget;
    ComPtr<IDirect3DSurface9> m_backbuffer;
    ComPtr<IDirect3DStateBlock9> m_stateBlock;

    D3DMATRIX m_projectionMatrix;
    D3DVIEWPORT9 m_viewport;
};

