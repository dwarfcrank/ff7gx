#include "stdafx.h"
#include "Renderer.h"

#include "Game.h"
#include "Module.h"

#include <assert.h>
#include <array>
#include <d3dcommon.h>
#include <DirectXMath.h>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#define VERIFY(hr) assert(SUCCEEDED((hr)))

using namespace DirectX;

namespace
{
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
}

// Helper templates to make wrapping methods as C functions easier
template<typename TResult, typename... TArgs>
using RendererMethodPtr = TResult(Renderer::*)(TArgs...);

template<typename TResult, typename... TArgs>
struct MethodWrapper
{
    template<RendererMethodPtr<TResult, TArgs...> Method>
    static TResult __cdecl Func(TArgs... args)
    {
        return (FF7::GetGfxContext()->rendererInstance->*Method)(args...);
    }
};

// Sets the name of a D3D9 resource, visible in a graphics debugger
static void SetD3DResourceName(IDirect3DResource9* resource, const char* name)
{
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, name, strlen(name), 0);
}

u32 Renderer::Shutdown(u32 a0)
{
    // TODO: Decide on shutdown order.
    // Currently it's original DLL shutdown -> Renderer shutdown -> Shutdown callback, which makes sense
    // when running with e.g. apitrace which isn't safe to unload before all D3D related stuff is destroyed.
    auto instance = FF7::GetGfxContext()->rendererInstance;
    auto origShutdown = instance->m_originalContext.Shutdown;
    auto ret = origShutdown(a0);
    auto shutdownCallback = instance->m_shutdownCallback;

    delete instance;

    if (shutdownCallback) {
        shutdownCallback();
    }

    return ret;
}

void Renderer::InitViewport()
{
    // The game sets the viewport min and max Z to 0.0f and 1.0f
    m_viewport.X = 0;
    m_viewport.Y = 0;
    m_viewport.Width = 320;
    m_viewport.Height = 240;
    m_viewport.MinZ = 0.0f;
    m_viewport.MaxZ = 1.0f;
}

void Renderer::InitProjectionMatrix()
{
    // Take the D3D9 pixel center offset into account when computing the projection matrix.
    auto matrix = XMMatrixOrthographicOffCenterLH(0.5f, 320.5f, 240.5, 0.5f, 0.0f, 1.0f);
    XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&m_projectionMatrix), matrix);
}

Renderer::Renderer(Module& module, FF7::GfxContext* context, ShutdownCallback shutdownCallback) :
    m_originalDll(module),
    m_internals(module),
    m_shutdownCallback(shutdownCallback)
{
    std::memcpy(&m_originalContext, context, sizeof(FF7::GfxContext));

    m_d3dDevice.Attach(m_internals.GetD3DDevice());

    // 3D models etc. are drawn directly to the backbuffer, so save it here to avoid
    // having to call GetRenderTarget() before switcing render targets
    VERIFY(m_d3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_backbuffer));
    SetD3DResourceName(m_backbuffer.Get(), "Backbuffer");

    // The backgrounds are drawn at ~320x240
    VERIFY(m_d3dDevice->CreateTexture(320, 240, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
        D3DPOOL_DEFAULT, &m_backgroundTexture, nullptr));
    SetD3DResourceName(m_backgroundTexture.Get(), "BackgroundTexture");

    VERIFY(m_backgroundTexture->GetSurfaceLevel(0, &m_backgroundRenderTarget));
    SetD3DResourceName(m_backgroundRenderTarget.Get(), "BackgroundRenderTarget");

    VERIFY(m_d3dDevice->CreateStateBlock(D3DSBT_ALL, &m_stateBlock));

    InitViewport();
    InitProjectionMatrix();

    context->DrawTiles = &MethodWrapper<void, void*, void*>::Func<&Renderer::DrawTiles>;
    context->EndFrame = &MethodWrapper<u32, u32>::Func<&Renderer::EndFrame>;
    context->Shutdown = Shutdown;
    context->Clear = &MethodWrapper<u32, u32, u32>::Func<&Renderer::Clear>;
    context->ClearAll = &MethodWrapper<u32>::Func<&Renderer::ClearAll>;

    // Patch DrawTilesImpl to call DrawHook to transform vertices before drawing them
    auto drawHook =
        &MethodWrapper<void, D3DPRIMITIVETYPE, u32, const FF7::Vertex*, u32, const u16*, u32, u32, u32>::Func<&Renderer::DrawHook>;
    m_originalDll.PatchCall(FF7::Offsets::TileDrawCall, static_cast<const void*>(drawHook));
}

void Renderer::DrawTiles(void* a0, void* a1)
{
    ScopedD3DEvent _(L"DrawTiles_hook(0x%p, 0x%p)", a0, a1);

    m_stateBlock->Capture();

    m_d3dDevice->SetTransform(D3DTS_PROJECTION, &m_projectionMatrix);
    m_d3dDevice->SetViewport(&m_viewport);

    m_d3dDevice->SetRenderTarget(0, m_backgroundRenderTarget.Get());

    // Depth writes have to be disabled to avoid interfering with other drawing done by the game
    m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

    m_originalContext.DrawTiles(a0, a1);

    m_stateBlock->Apply();

    // The render target is not saved with the state block, so restore it separately
    m_d3dDevice->SetRenderTarget(0, m_backbuffer.Get());
}

void Renderer::DrawHook(D3DPRIMITIVETYPE primType, u32 drawType, const FF7::Vertex* vertices,
    u32 vertexBufferSize, const u16* indices, u32 vertexCount, u32 a7, u32 scissor)
{
    std::vector<FF7::Vertex> transformed(vertexBufferSize);

    // The original vertices are generated for a 640x480 render target, so they
    // need to be scaled to 320x240, otherwise only the upper left corner of the background is rendered.
    // TODO: Check if using the game's own projection matrix would work.
    for (u32 i = 0; i < vertexBufferSize; i++) {
        transformed[i] = vertices[i];
        transformed[i].x *= 0.5f;
        transformed[i].y *= 0.5f;
    }

    m_internals.Draw(primType, drawType, transformed.data(), vertexBufferSize, indices, vertexCount, a7, scissor);
}

void Renderer::SetShaderTextureFlag(bool value)
{
    float v[4] = { value ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };

    // texture_flag is in register c1
    m_d3dDevice->SetPixelShaderConstantF(1, v, 1);
}

u32 Renderer::EndFrame(u32 a0)
{
    ScopedD3DEvent _(L"EndFrame_hook(0x%p)", a0);

    float width, height;
    m_internals.GetRenderDimensions(&width, &height);
    width /= 2.0f;
    height /= 2.0f;

    const std::array<u16, 6> indices{ {
            3, 0, 2, 0, 1, 2
        } };

    const std::array<FF7::Vertex, 4> vertices{ {
        {
            width, 0.0f, 1.0f, 1.0f,    // x, y, z, w
            0xffffffff, 0.0f,           // color, unknown,
            1.0f, 0.0f                  // u, v
        },
        {
            width, height, 1.0f, 1.0f,  // x, y, z, w
            0xffffffff, 0.0f,           // color, unknown,
            1.0f, 1.0f                  // u, v
        },
        {
            0.0f, height, 1.0f, 1.0f,   // x, y, z, w
            0xffffffff, 0.0f,           // color, unknown,
            0.0f, 1.0f                  // u, v
        },
        {
            0.0f, 0.0f, 1.0f, 1.0f,     // x, y, z, w
            0xffffffff, 0.0f,           // color, unknown,
            0.0f, 0.0f                  // u, v
        }
        } };

    m_d3dDevice->SetRenderTarget(0, m_backbuffer.Get());
    m_d3dDevice->SetTexture(0, m_backgroundTexture.Get());

    m_internals.SetTextureFilteringFlag(1);
    SetShaderTextureFlag(true);

    m_d3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    m_internals.Draw(D3DPT_TRIANGLELIST, FF7::DrawType::Ortho, vertices.data(), vertices.size(), indices.data(), indices.size(), 0, 0);
    m_d3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);

    return m_originalContext.EndFrame(a0);
}

u32 Renderer::Clear(u32 clearRenderTarget, u32 clearDepthBuffer)
{
    m_d3dDevice->SetRenderTarget(0, m_backgroundRenderTarget.Get());
    m_originalContext.Clear(clearRenderTarget, clearDepthBuffer);
    m_d3dDevice->SetRenderTarget(0, m_backbuffer.Get());

    return m_originalContext.Clear(clearRenderTarget, clearDepthBuffer);
}

u32 Renderer::ClearAll()
{
    return Clear(1, 1);
}