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
    int spawnCount;
    int stateID;
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
};


// ==============================================================================
// PIPELINE CLASS
// ==============================================================================

class ComputePipeline
{
public:
    static constexpr UINT MAX_BULLETS = 100000;

    HRESULT Initialize(Renderer& renderer);
    void Shutdown();

    void Dispatch(Renderer& renderer, const GlobalConstants& constants);

    ID3D12Resource* GetBulletBuffer() const { return m_bulletBuffer.Get(); }
    ID3D12DescriptorHeap* GetCbvSrvUavHeap() const { return m_cbvSrvUavHeap.Get(); }

private:
	// Core DX12 Pipeline Components
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;

	ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
    UINT m_descriptorSize = 0;

	ComPtr<ID3D12Resource> m_bulletBuffer; // Structured Buffer for Bullet Data (UAV)

    HRESULT CreateRootSignature_(Renderer& renderer);
    HRESULT CreatePSO_(Renderer& renderer);
    HRESULT CreateBulletBufferAndViews_(Renderer& renderer);
};