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
        ScopedD3DEvent()
        {
        }

        ~ScopedD3DEvent()
        {
            D3DPERF_EndEvent();
        }

        static ScopedD3DEvent Begin(const WCHAR* format, ...)
        {
            WCHAR buf[128];

            va_list args;
            va_start(args, format);
            std::vswprintf(buf, 128, format, args);
            va_end(args);

            D3DPERF_BeginEvent(0, buf);

            return ScopedD3DEvent();
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
        return (Renderer::GetInstance()->*Method)(args...);
    }
};

// Sets the name of a D3D9 resource, visible in a graphics debugger
static void SetD3DResourceName(IDirect3DResource9* resource, const char* name)
{
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, name, strlen(name), 0);
}

std::unique_ptr<Renderer> Renderer::s_instance = nullptr;

Renderer* Renderer::GetInstance()
{
    assert(s_instance);
    return s_instance.get();
}

Renderer* Renderer::Initialize(Module& module, FF7::GfxContext* context, ShutdownCallback shutdownCallback)
{
    assert(!s_instance);
    s_instance.reset(new Renderer(module, context, shutdownCallback));

    return s_instance.get();
}

u32 Renderer::Shutdown(u32 a0)
{
    // TODO: Decide on shutdown order.
    // Currently it's original DLL shutdown -> Renderer shutdown -> Shutdown callback, which makes sense
    // when running with e.g. apitrace which isn't safe to unload before all D3D related stuff is destroyed.
    auto origShutdown = GetInstance()->m_originalContext.Shutdown;
    auto ret = origShutdown(a0);
    auto shutdownCallback = GetInstance()->m_shutdownCallback;

    s_instance.reset();

    if (shutdownCallback) {
        shutdownCallback();
    }

    return ret;
}

Renderer::Renderer(Module& module, FF7::GfxContext* context, ShutdownCallback shutdownCallback) :
    m_originalDll(module),
    m_shutdownCallback(shutdownCallback)
{
    std::memcpy(&m_originalContext, context, sizeof(FF7::GfxContext));

    m_d3dDevice.Attach(*m_originalDll.OffsetToPtr<IDirect3DDevice9**>(FF7::Offsets::D3DDevice));

    // 3D models etc. are drawn directly to the backbuffer, so save it here to avoid
    // having to call GetRenderTarget() before switcing render targets
    VERIFY(m_d3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_backbuffer));
    SetD3DResourceName(m_backbuffer.Get(), "Backbuffer");

    // The backgrounds are drawn at ~320x240
    VERIFY(m_d3dDevice->CreateTexture(320, 240, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
        D3DPOOL_DEFAULT, &m_renderTargetTexture, nullptr));
    SetD3DResourceName(m_renderTargetTexture.Get(), "RenderTargetTexture");

    VERIFY(m_renderTargetTexture->GetSurfaceLevel(0, &m_renderTargetSurface));
    SetD3DResourceName(m_renderTargetSurface.Get(), "RenderTargetSurface");

    context->DrawTiles = &MethodWrapper<void, void*, void*>::Func<&Renderer::DrawTiles>;
    context->EndFrame = &MethodWrapper<u32, u32>::Func<&Renderer::EndFrame>;
    context->Shutdown = Shutdown;
    context->Clear = &MethodWrapper<u32, u32, u32>::Func<&Renderer::Clear>;
    context->ClearAll = &MethodWrapper<u32>::Func<&Renderer::ClearAll>;

    Draw = m_originalDll.OffsetToPtr<decltype(Draw)>(FF7::Offsets::DrawFunction);

    // Patch DrawTilesImpl to call DrawHook to transform vertices before drawing them
    auto drawHook =
        &MethodWrapper<void, D3DPRIMITIVETYPE, u32, const FF7::Vertex*, u32, const u16*, u32, u32, u32>::Func<&Renderer::DrawHook>;
    m_originalDll.PatchCall(FF7::Offsets::TileDrawCall, static_cast<const void*>(drawHook));
}

void Renderer::DrawTiles(void* a0, void* a1)
{
    auto _ = ScopedD3DEvent::Begin(L"DrawTiles_hook(%p, %p)", a0, a1);

    // Take the D3D9 pixel center offset into account when computing the projection matrix.
    // TODO: Compute this once and save it.
    auto matrix = XMMatrixOrthographicOffCenterLH(0.5f, 320.5f, 240.5, 0.5f, 0.0f, 1.0f);
    XMFLOAT4X4 temp;
    XMStoreFloat4x4(&temp, matrix);

    D3DMATRIX oldMatrix;
    m_d3dDevice->GetTransform(D3DTS_PROJECTION, &oldMatrix);
    m_d3dDevice->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(&temp));

    D3DVIEWPORT9 viewport, oldViewport;
    m_d3dDevice->GetViewport(&oldViewport);

    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = 320;
    viewport.Height = 240;
    viewport.MinZ = oldViewport.MinZ;
    viewport.MaxZ = oldViewport.MaxZ;

    m_d3dDevice->SetViewport(&viewport);

    m_d3dDevice->SetRenderTarget(0, m_renderTargetSurface.Get());

    // Depth writes have to be disabled to avoid interfering with other drawing done by the game
    // TODO: Maybe use a state block to save all state?
    m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

    m_originalContext.DrawTiles(a0, a1);

    m_d3dDevice->SetRenderTarget(0, m_backbuffer.Get());
    m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

    m_d3dDevice->SetViewport(&oldViewport);
    m_d3dDevice->SetTransform(D3DTS_PROJECTION, &oldMatrix);
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

    Draw(primType, drawType, transformed.data(), vertexBufferSize, indices, vertexCount, a7, scissor);
}

void Renderer::SetTextureFilteringFlag(bool value)
{
    *m_originalDll.OffsetToPtr<u32*>(FF7::Offsets::TextureFilteringFlag) = value ? 1 : 0;
}

void Renderer::SetTextureFlag(bool value)
{
    float v[4] = { value ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };

    // texture_flag is in register c1
    m_d3dDevice->SetPixelShaderConstantF(1, v, 1);
}

u32 Renderer::EndFrame(u32 a0)
{
    auto _ = ScopedD3DEvent::Begin(L"EndFrame_hook(%p)", a0);

    auto width = static_cast<float>(*m_originalDll.OffsetToPtr<u32*>(FF7::Offsets::RenderWidth)) / 2;
    auto height = static_cast<float>(*m_originalDll.OffsetToPtr<u32*>(FF7::Offsets::RenderHeight)) / 2;

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
    m_d3dDevice->SetTexture(0, m_renderTargetTexture.Get());

    SetTextureFilteringFlag(true);
    SetTextureFlag(true);

    m_d3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    Draw(D3DPT_TRIANGLELIST, FF7::DrawType::Ortho, vertices.data(), vertices.size(), indices.data(), indices.size(), 0, 0);
    m_d3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);

    return m_originalContext.EndFrame(a0);
}

u32 Renderer::Clear(u32 clearRenderTarget, u32 clearDepthBuffer)
{
    m_d3dDevice->SetRenderTarget(0, m_renderTargetSurface.Get());
    m_originalContext.Clear(clearRenderTarget, clearDepthBuffer);
    m_d3dDevice->SetRenderTarget(0, m_backbuffer.Get());

    return m_originalContext.Clear(clearRenderTarget, clearDepthBuffer);
}

u32 Renderer::ClearAll()
{
    return Clear(1, 1);
}