#include "stdafx.h"
#include "Game.h"
#include "Module.h"

namespace FF7
{
    GameContext* GetGameContext()
    {
        return *reinterpret_cast<GameContext**>(Offsets::GameContextPtr);
    }

    GfxContext* GetGfxContext()
    {
        auto contextPtr = reinterpret_cast<u32*>(GetGameContext());

        // GfxContext* is at GameContext+0x934
        return reinterpret_cast<GfxContext*>(contextPtr[0x934 / 4]);
    }

    GameInternals::GameInternals(Module& module) :
        m_module(module)
    {
        m_draw = m_module.OffsetToPtr<decltype(m_draw)>(Offsets::DrawFunction);
        m_getGameState = m_module.OffsetToPtr<decltype(m_getGameState)>(Offsets::GetGameState);
    }

    IDirect3DDevice9* GameInternals::GetD3DDevice()
    {
        return *m_module.OffsetToPtr<IDirect3DDevice9**>(Offsets::D3DDevice);
    }

    void GameInternals::GetRenderDimensions(float* width, float* height) const
    {
        *width = static_cast<float>(*m_module.OffsetToPtr<const u32*>(Offsets::RenderWidth));
        *height = static_cast<float>(*m_module.OffsetToPtr<const u32*>(Offsets::RenderHeight));
    }

    const GameState* GameInternals::GetGameState() const
    {
        return m_getGameState();
    }

    void GameInternals::SetDebugLogFlag(u32 value)
    {
        *m_module.OffsetToPtr<u32*>(Offsets::DebugLogFlag) = value;
    }

    void GameInternals::SetDebugOverlayFlag(u32 value)
    {
        *m_module.OffsetToPtr<u32*>(Offsets::DebugOverlayFlag) = value;
    }

    void GameInternals::SetTextureFilteringFlag(u32 value)
    {
        *m_module.OffsetToPtr<u32*>(Offsets::TextureFilteringFlag) = value;
    }

    void GameInternals::Draw(D3DPRIMITIVETYPE primType, u32 drawType, const Vertex* vertices, u32 vertexBufferSize,
        const u16* indices, u32 vertexCount, u32 a7, u32 scissor)
    {
        m_draw(primType, drawType, vertices, vertexBufferSize, indices, vertexCount, a7, scissor);
    }

}
