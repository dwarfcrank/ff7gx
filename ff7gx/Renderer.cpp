#include "stdafx.h"
#include "Renderer.h"

#include "Game.h"
#include "Module.h"
#include "ScopedD3DEvent.h"

#include <assert.h>
#include <array>
#include <d3dcommon.h>
#include <DirectXMath.h>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <algorithm>
#include <cmath>

#include "Generated/Background_PS.h"
#include "Generated/BackgroundLayer_PS.h"
#include "Generated/Background_VS.h"

#define VERIFY(hr) assert(SUCCEEDED((hr)))

using namespace DirectX;

// Helper templates to make wrapping methods as C functions easier
template<typename TResult, typename... TArgs>
using RendererMethodPtr = TResult(Renderer::*)(TArgs...);

template<typename TResult, typename... TArgs>
struct MethodWrapper
{
    template<RendererMethodPtr<TResult, TArgs...> Method>
    static TResult __cdecl Func(TArgs... args)
    {
        return (static_cast<Renderer*>(FF7::GetGfxFunctions()->rendererInstance)->*Method)(args...);
    }
};

// Sets the name of a D3D9 resource, visible in a graphics debugger
static void SetD3DResourceName(IDirect3DResource9* resource, const char* name)
{
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, name, strlen(name), 0);
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

void Renderer::DrawLayers()
{
    m_stateBlock->Capture();

    const std::array<u16, 6> indices{ {
            3, 0, 2, 0, 1, 2
        } };

    float width, height;
    m_internals.GetRenderDimensions(&width, &height);
    width /= 2.0f;
    height /= 2.0f;

    std::vector<int> layers(m_layerDepths.cbegin(), m_layerDepths.cend());
    std::sort(layers.begin(), layers.end());

    auto oldVS = m_internals.GetTlmainVS();
    m_internals.SetTlmainVS(m_backgroundVS.Get());

    m_d3dDevice->SetRenderTarget(0, m_backbuffer.Get());
    m_d3dDevice->SetTexture(0, m_backgroundTexture.Get());
    m_d3dDevice->SetTexture(1, m_backgroundTexture.Get());
    m_d3dDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    m_d3dDevice->SetPixelShader(m_backgroundLayerPS.Get());
    m_internals.SetTextureFilteringFlag(1);
    m_d3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

    for (auto it = layers.crbegin(); it != layers.crend(); it++) {
        const float depth = static_cast<float>(*it / 255.0f);

        const std::array<FF7::Vertex, 4> vertices{ {
            {
                width, 0.0f, depth, 1.0f,    // x, y, z, w
                0xffffffff, 0.0f,           // color, unknown,
                1.0f, 0.0f                  // u, v
            },
            {
                width, height, depth, 1.0f,  // x, y, z, w
                0xffffffff, 0.0f,           // color, unknown,
                1.0f, 1.0f                  // u, v
            },
            {
                0.0f, height, depth, 1.0f,   // x, y, z, w
                0xffffffff, 0.0f,           // color, unknown,
                0.0f, 1.0f                  // u, v
            },
            {
                0.0f, 0.0f, depth, 1.0f,     // x, y, z, w
                0xffffffff, 0.0f,           // color, unknown,
                0.0f, 0.0f                  // u, v
            }
            } };

        float psConstant[4] = { std::roundf(static_cast<float>(*it)), 0.0f, 0.0f, 0.0f };
        m_d3dDevice->SetPixelShaderConstantF(0, psConstant, 1);
        m_internals.Draw(D3DPT_TRIANGLELIST, FF7::DrawType::Ortho, vertices.data(), vertices.size(), indices.data(), indices.size(), 0, 0);
    }

    m_internals.SetTlmainVS(oldVS);
    m_stateBlock->Apply();
}

Renderer::Renderer(Module& module, FF7::GfxFunctions* functions) :
    GfxContextBase(functions),
    m_drawMode(DrawMode::Dialog),
    m_originalDll(module),
    m_internals(module)
{
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

    VERIFY(m_d3dDevice->CreatePixelShader(reinterpret_cast<const DWORD*>(Background_PS), &m_backgroundPS));
    VERIFY(m_d3dDevice->CreatePixelShader(reinterpret_cast<const DWORD*>(BackgroundLayer_PS), &m_backgroundLayerPS));
    VERIFY(m_d3dDevice->CreateVertexShader(reinterpret_cast<const DWORD*>(Background_VS), &m_backgroundVS));

    InitViewport();
    InitProjectionMatrix();

    // Patch DrawTilesImpl to call DrawHook to transform vertices before drawing them
    auto drawHook =
        &MethodWrapper<void, D3DPRIMITIVETYPE, u32, const FF7::Vertex*, u32, const u16*, u32, u32, u32>::Func<&Renderer::DrawHook>;
    m_originalDll.PatchCall(FF7::Offsets::TileDrawCall, static_cast<const void*>(drawHook));
}

void Renderer::DrawTiles(void* a0, void* a1)
{
    if (m_drawMode != DrawMode::Background) {
        // Not drawing background tiles, just draw normally.
        GfxContextBase::DrawTiles(a0, a1);
        return;
    }

    ScopedD3DEvent _(L"DrawTiles_hook(0x%p, 0x%p)", a0, a1);

    m_stateBlock->Capture();

    m_d3dDevice->SetTransform(D3DTS_PROJECTION, &m_projectionMatrix);
    m_d3dDevice->SetViewport(&m_viewport);

    m_d3dDevice->SetRenderTarget(0, m_backgroundRenderTarget.Get());
    m_d3dDevice->SetPixelShader(m_backgroundPS.Get());

    // Depth writes have to be disabled to avoid interfering with other drawing done by the game
    m_d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    m_d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    // Draw() is stupid and sets the vertex shader on every call, so we need to swap out the original
    // object OR rewrite the function.
    auto oldVS = m_internals.GetTlmainVS();
    m_internals.SetTlmainVS(m_backgroundVS.Get());
    GfxContextBase::DrawTiles(a0, a1);
    m_internals.SetTlmainVS(oldVS);

    m_stateBlock->Apply();

    // The render target is not saved with the state block, so restore it separately
    m_d3dDevice->SetRenderTarget(0, m_backbuffer.Get());
}

void Renderer::DrawHook(D3DPRIMITIVETYPE primType, u32 drawType, const FF7::Vertex* vertices,
    u32 vertexBufferSize, const u16* indices, u32 vertexCount, u32 a7, u32 scissor)
{
    if (m_drawMode != DrawMode::Background) {
        // Not drawing background tiles, just draw normally.
        m_internals.Draw(primType, drawType, vertices, vertexBufferSize, indices, vertexCount, a7, scissor);
        return;
    }

    std::vector<FF7::Vertex> transformed(vertexBufferSize);

    // The original vertices are generated for a 640x480 render target, so they
    // need to be scaled to 320x240, otherwise only the upper left corner of the background is rendered.
    // TODO: Check if using the game's own projection matrix would work.
    for (u32 i = 0; i < vertexBufferSize; i++) {
        transformed[i] = vertices[i];
        transformed[i].x *= 0.5f;
        transformed[i].y *= 0.5f;

        m_layerDepths.insert(static_cast<int>(std::ceilf(transformed[i].z * 255.0f)));
    }

    m_internals.Draw(primType, drawType, transformed.data(), vertexBufferSize, indices, vertexCount, a7, scissor);
}

void Renderer::GfxFn_84(u32 drawMode, FF7::GameContext* context)
{
    auto gameMode = m_internals.GetGameState()->mode;

    if (gameMode == FF7::GameMode::Field) {
        if (drawMode == 1) {
            m_drawMode = DrawMode::Dialog;
        }
    } else {
        // At least in main menu dialogs are drawn with drawMode = 0
        if (drawMode == 0) {
            m_drawMode = DrawMode::Dialog;
        }
    }

    GfxContextBase::GfxFn_84(drawMode, context);
}

u32 Renderer::GfxFn_88(u32 drawMode, FF7::GameContext* context)
{
    auto ret = GfxContextBase::GfxFn_88(drawMode, context);

    if (ret) {
        if (drawMode == 0) {
            m_drawMode = DrawMode::Background;
        } else if (drawMode == 1) {
            m_drawMode = DrawMode::Dialog;
        }
    }

    return ret;
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

    DrawLayers();

    m_layerDepths.clear();

    return GfxContextBase::EndFrame(a0);
}

u32 Renderer::Clear(u32 clearRenderTarget, u32 clearDepthBuffer)
{
    m_d3dDevice->SetRenderTarget(0, m_backgroundRenderTarget.Get());
    GfxContextBase::Clear(clearRenderTarget, clearDepthBuffer);
    m_d3dDevice->SetRenderTarget(0, m_backbuffer.Get());

    return GfxContextBase::Clear(clearRenderTarget, clearDepthBuffer);
}

u32 Renderer::ClearAll()
{
    return Clear(1, 1);
}