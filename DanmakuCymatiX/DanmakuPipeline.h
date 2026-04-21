#pragma once
#include "Renderer.h"
#include <cstdint>

// ==============================================================================
// DATA STRUCTS
// ==============================================================================

struct alignas(16) BulletGPU
{
    float posX;
    float posY;
    float velX;
    float velY;

    float spawnTime;
    uint32_t state;
    float baseRadius;
    float spikes;
};

// GPU Payload
struct alignas(16) GlobalConstants
{
	// Data from Stochastic Engine
	uint32_t packedStateAndSpawn; // 16 bits for state ID, 16 bits for spawn count (Bit Packing). Spawn Limit: 65535 per Frame
	uint32_t spawnStartIndex; // For the ring buffer of bullets (Poisson distribution)
    float sweepFactor;
    float chaosFactor;

    float deltaTime;

    float originX;
    float originY;
    float spatialSpread;
    float totalTime;

    // Data from the game
    float playerPosX;
    float playerPosY;

    float hitRadius;
    float grazeRadius;

    float band1;
	float band2;
    float band3;
};

// ==============================================================================
// PIPELINE CLASS
// ==============================================================================

class DanmakuPipeline
{
public:

    static constexpr UINT MAX_BULLETS = 100000;

    HRESULT Initialize(Renderer& renderer);
    void Shutdown();

#pragma region COMPUTE PIPELINE

public:

    void Dispatch(Renderer& renderer, const GlobalConstants& constants);

    ID3D12Resource* GetBulletBuffer() const { return m_bulletBuffer.Get(); }
    ID3D12DescriptorHeap* GetCbvSrvUavHeap() const { return m_cbvSrvUavHeap.Get(); }

private:
    ComPtr<ID3D12RootSignature> m_computeRS;
    ComPtr<ID3D12PipelineState> m_computePSO;

    ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
    UINT m_descriptorSize = 0;

    ComPtr<ID3D12Resource> m_bulletBuffer; // Structured Buffer for Bullet Data (UAV)

    HRESULT CreateComputeRootSignature_(Renderer& renderer);
    HRESULT CreateComputePSO_(Renderer& renderer);
    HRESULT CreateBulletBufferAndViews_(Renderer& renderer);

#pragma endregion

#pragma region GRAPHICS PIPELINE

public:

    void Render(Renderer& renderer);

private:
    ComPtr<ID3D12RootSignature> m_graphicsRS;
    ComPtr<ID3D12PipelineState> m_graphicsPSO;

    HRESULT CreateGraphicsRootSignature_(Renderer& renderer);
    HRESULT CreateGraphicsPSO_(Renderer& renderer);

#pragma endregion
};

