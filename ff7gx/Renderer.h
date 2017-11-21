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

    // The callback passed here is run *after* the renderer instance has been destroyed.
    static Renderer* Initialize(class Module& Module, FF7::GfxContext* context, ShutdownCallback shutdownCallback = nullptr);
    static Renderer* GetInstance();

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
    // std::default_delete needs to be a friend to access the private destructor
    friend struct std::default_delete<Renderer>;
    static std::unique_ptr<Renderer> s_instance;

    Renderer(class Module& module, FF7::GfxContext* context, ShutdownCallback shutdownCallback = nullptr);
    ~Renderer() = default;

    // Sets the flag used by the pixel shader to determine whether to sample from a texture.
    void SetTextureFlag(bool value);

    // Sets the flag used by Draw() to determine whether to enable linear filtering or not.
    void SetTextureFilteringFlag(bool value);

    ShutdownCallback m_shutdownCallback;

    // Game internals
    class Module& m_originalDll;
    FF7::GfxContext m_originalContext;

    // Original drawing function, see FF7::DrawType for the values of drawType
    void (__cdecl *Draw)(D3DPRIMITIVETYPE primType, u32 drawType, const FF7::Vertex* vertices,
        u32 vertexBufferSize, const u16* indices, u32 vertexCount, u32 a7, u32 scissor);

    // D3D resources
    template<typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<IDirect3DDevice9> m_d3dDevice;
    ComPtr<IDirect3DTexture9> m_renderTargetTexture;
    ComPtr<IDirect3DSurface9> m_renderTargetSurface;
    ComPtr<IDirect3DSurface9> m_backbuffer;
};

