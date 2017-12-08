#pragma once

#include "Game.h"
#include "GfxContextBase.h"

#include <d3d9.h>
#include <functional>
#include <memory>
#include <Windows.h>
#include <wrl.h>
#include <unordered_set>

class Renderer : public GfxContextBase
{
public:
    Renderer(class Module& module, FF7::GfxFunctions* functions);

    virtual u32 EndFrame(u32 a0) override;
    virtual u32 Clear(u32 clearRenderTarget, u32 clearDepthBuffer) override;
    virtual u32 ClearAll() override;
    virtual void DrawTiles(void* a0, void* a1) override;

    // DrawTilesImpl is patched to call this instead of the original Draw()
    void DrawHook(D3DPRIMITIVETYPE primType, u32 drawType, const FF7::Vertex* vertices,
        u32 vertexBufferSize, const u16* indices, u32 vertexCount, u32 a7, u32 scissor);

    virtual void GfxFn_84(u32 drawMode, FF7::GameContext* context) override;
    virtual u32 GfxFn_88(u32 drawMode, FF7::GameContext* context) override;

    Renderer(Renderer&) = delete;
    Renderer(Renderer&&) = delete;

private:
    // DrawMode is used to determine what part of the scene the game is currently drawing.
    // This affects z-buffering and blending among others.
    enum DrawMode
    {
        Background, // Draw to a separate texture and upscale later
        Dialog      // Draw directly to back buffer
    };

    // Sets the flag used by the pixel shader to determine whether to sample from a texture.
    void SetShaderTextureFlag(bool value);

    // Sets the flag used by Draw() to determine whether to enable linear filtering or not.
    void SetTextureFilteringFlag(bool value);

    void InitViewport();
    void InitProjectionMatrix();

    void DrawLayers();

    // Drawing state
    DrawMode m_drawMode;
    std::unordered_set<int> m_layerDepths;

    // Game internals
    class Module& m_originalDll;
    FF7::GameInternals m_internals;

    // D3D resources
    template<typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ComPtr<IDirect3DDevice9> m_d3dDevice;
    ComPtr<IDirect3DTexture9> m_backgroundTexture;
    ComPtr<IDirect3DSurface9> m_backgroundRenderTarget;
    ComPtr<IDirect3DSurface9> m_backbuffer;
    ComPtr<IDirect3DStateBlock9> m_stateBlock;

    ComPtr<IDirect3DPixelShader9> m_backgroundLayerPS;
    ComPtr<IDirect3DPixelShader9> m_backgroundPS;
    ComPtr<IDirect3DVertexShader9> m_backgroundVS;

    D3DMATRIX m_projectionMatrix;
    D3DVIEWPORT9 m_viewport;
};

